/**
 * @file ZmqMessageFullImpl.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Full definition of all ZmqMessage objects methods.
 * In HEADERONLY builds, they are included in client code.
 * Default (explicit) instantiations of them are included in library code,
 * so in non-headeronly builds this code does not appear in client modules.
 */

#ifndef ZMQMESSAGE_ZMQMESSAGEFULLIMPL_HPP_
#define ZMQMESSAGE_ZMQMESSAGEFULLIMPL_HPP_

#include <tr1/functional>

namespace ZmqMessage
{
  void
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock,
    SendObserver* send_observer)
    throw(ZmqErrorType)
  {
    int base_flags = nonblock ? ZMQ_NOBLOCK : 0;
    for (size_t i = 0; i < multipart.size(); ++i)
    {
      int flags = base_flags | ((i < multipart.size()-1) ? ZMQ_SNDMORE : 0);
      if (send_observer)
      {
        send_observer->on_send_part(multipart.parts_[i].msg());
      }
      send_msg(sock, multipart.parts_[i].msg(), flags);
    }
    if (send_observer)
    {
      send_observer->on_flush();
    }
  }

  void
  Multipart::clear()
  {
    parts_.clear();
  }

  Multipart*
  Multipart::detach()
  {
    std::auto_ptr<Multipart> m(new Multipart());
    m->reserve(parts_.size());
    m->parts_ = parts_; //move structs, current object's parts become invalid
    return m.release();
  }

  void
  Multipart::check_has_part(size_t n) const throw(NoSuchPartError)
  {
    if (n >= size())
    {
      std::ostringstream ss;
      ss << "Multipart zmq message has only " << size() <<
          " parts received, but we requested index: " << n;
      throw NoSuchPartError(ss.str());
    }
    if (!parts_[n].valid())
    {
      std::ostringstream ss;
      ss << "Multipart zmq message at " << n <<
          " is not owned by this multipart";
      throw NoSuchPartError(ss.str());
    }
  }

  Part&
  Multipart::operator[](size_t i) throw (NoSuchPartError)
  {
    check_has_part(i);
    return parts_[i];
  }

  Part
  Multipart::release(size_t i)
  {
    if (i >= size())
    {
      return Part(false);
    }
    return parts_[i]; //copy
  }

  void
  XRouting::log_routing_received() const
  {
    ZMQMESSAGE_LOG_STREAM << "Receiving multipart, route received: "
      << routing_.size() << " parts";
  }

  void
  XRouting::receive_routing(zmq::socket_t& sock)
    throw (MessageFormatError, ZmqErrorType)
  {
    if (!routing_.empty())
    {
      return;
    }
    routing_.reserve(3); //should be enough for most cases
    for (int i = 0; ; ++i)
    {
      routing_.push_back(Part());
      Part& part = routing_.back();

      recv_msg(sock, part.msg());
      ZMQMESSAGE_LOG_STREAM <<
        "Received X route: " << part.msg().size() << "bytes;" <<
        ZMQMESSAGE_LOG_TERM;

      if (part.msg().size() == 0)
      {
        break;
      }
      if (!has_more(sock))
      {
        std::ostringstream ss;
        ss << "Receiving multipart message: reading route info failed: "
            << "part " << (i+1) << " has nothing after it. "
            << "Routing info doesn't end with null message";
        throw MessageFormatError(ss.str());
      }
    }
  }

  template <class RoutingPolicy>
  void
  Incoming<RoutingPolicy>::check_is_terminal() const throw(MessageFormatError)
  {
    if (!is_terminal_)
    {
      std::ostringstream ss;
      ss <<
        "Receiving multipart "
        "Has more messages after part " << size() << ", but must be terminal";
      throw MessageFormatError(ss.str());
    }
  }

  template <class RoutingPolicy>
  void
  Incoming<RoutingPolicy>::append_message_data(
    zmq::message_t& message, std::vector<char>& area) const
  {
    std::vector<char>::size_type sz = area.size();
    area.resize(sz + message.size());
    std::copy(static_cast<char*>(message.data()),
              static_cast<char*>(message.data()) + message.size(),
              area.begin() + sz
    );
  }

  template <class RoutingPolicy>
  bool
  Incoming<RoutingPolicy>::receive_one() throw(ZmqErrorType)
  {
    parts_.push_back(Part());
    Part& cur_part = parts_.back();

    const bool more = do_receive_msg(cur_part);

    ZMQMESSAGE_LOG_STREAM << "Incoming received "
      << cur_part.msg().size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)cur_part.msg().data(),
        std::min(cur_part.msg().size(), static_cast<size_t>(256)))
      << ", has more = " << more << ZMQMESSAGE_LOG_TERM;
    return more;
  }

  template <class RoutingPolicy>
  void
  Incoming<RoutingPolicy>::validate(
    const char* part_names[],
    size_t part_names_length, bool strict)
    throw (MessageFormatError)
  {
    if (parts_.size() < part_names_length)
    {
      std::ostringstream ss;
      ss <<
        "Validating received multipart: "
        "No more messages after " << part_names[parts_.size() - 1] <<
        "(" << (parts_.size()) << "), expected " <<
        (strict ? "exactly" : "at least") << " " << part_names_length;
      throw MessageFormatError(ss.str());
    }
    if (strict && parts_.size() > part_names_length)
    {
      std::ostringstream ss;
      ss <<
        "Validating received multipart: "
        "Have received " << parts_.size() << " parts, while expected "
        "exactly " << part_names_length;
      throw MessageFormatError(ss.str());
    }
  }

  template <class RoutingPolicy>
  Incoming <RoutingPolicy>&
  Incoming<RoutingPolicy>::receive(
      size_t parts, const char* part_names[],
      size_t part_names_length, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType)
  {
    RoutingPolicy::receive_routing(src_);
    RoutingPolicy::log_routing_received();

    for (size_t i = 0, init_parts = size(); i < parts; ++i)
    {
      bool more = receive_one();
      const char* const part_name =
        (i < part_names_length) ? part_names[i] : "<unnamed>";

      if (i < parts - 1 && !more)
      {
        is_terminal_ = true;
        std::ostringstream ss;
        ss <<
          "Receiving multipart: "
          "No more messages after " << part_name <<
          "(" << (init_parts + i) << "), expected more";
        throw MessageFormatError(ss.str());
      }
      if (i == parts - 1 && more)
      {
        is_terminal_= false;
        if (check_terminal)
        {
          std::ostringstream ss;
          ss <<
            "Receiving multipart: "
            "Has more messages after " << part_name <<
            "(" << (init_parts + i) << "), expected no more messages";
          throw MessageFormatError(ss.str());
        }
      }
      if (i == parts - 1 && !more)
      {
        is_terminal_ = true;
      }
    }
    return *this;
  }

  template <class RoutingPolicy>
  Incoming <RoutingPolicy>&
  Incoming<RoutingPolicy>::receive_all(
      size_t min_parts, const char* part_names[], size_t part_names_length)
      throw (MessageFormatError, ZmqErrorType)
  {
    receive(min_parts, part_names, part_names_length, false);

    while (!is_terminal_)
    {
      is_terminal_ = !receive_one();
    }

    return *this;
  }

  template <class RoutingPolicy>
  Incoming <RoutingPolicy>&
  Incoming<RoutingPolicy>::receive_up_to(
    size_t min_parts,
    const char* part_names[], size_t max_parts)
    throw (MessageFormatError, ZmqErrorType)
  {
    receive(min_parts, part_names, false);

    for (size_t n = min_parts; n < max_parts && !is_terminal_; ++n)
    {
      is_terminal_ = !receive_one();
    }

    return *this;
  }

  template <class RoutingPolicy>
  int
  Incoming<RoutingPolicy>::fetch_tail(
      std::vector<char>& area, const char* delimiter) throw (ZmqErrorType)
  {
    append_message_data(parts_.back().msg(), area);
    if (is_terminal_)
    {
      return 1;
    }

    size_t delim_sz = (delimiter) ? ::strlen(delimiter) : 0;

    int num_messages = 1;
    for (bool more = has_more(src_); more; ++num_messages)
    {
      if (delim_sz)
      {
        std::vector<char>::size_type sz = area.size();
        area.resize(sz + delim_sz);
        std::copy(delimiter, delimiter + delim_sz, area.begin() + sz);
      }
      Part data_buff;
      more = do_receive_msg(data_buff);
      append_message_data(data_buff.msg(), area);
    }
    return num_messages;
  }

  template <class RoutingPolicy>
  int
  Incoming<RoutingPolicy>::drop_tail() throw(ZmqErrorType)
  {
    if (is_terminal_)
    {
      return 0;
    }

    Part data_buff;
    bool more = false;
    int num_messages = 0;
    if (parts_.empty()) //we haven't received anything
    {
      if (!try_recv_msg(src_, data_buff.msg(), ZMQ_NOBLOCK))
      {
        return 0;
      }
      more = has_more(src_);
      if (receive_observer_)
      {
        receive_observer_->on_receive_part(data_buff.msg(), more);
      }
      num_messages = 1;
    }
    else
    {
      more = has_more(src_);
    }

    for (; more; ++num_messages)
    {
      more = do_receive_msg(data_buff);
    }
    return num_messages;
  }

  template <class RoutingPolicy>
  Incoming<RoutingPolicy>&
  Incoming<RoutingPolicy>::operator>> (
    zmq::message_t& msg) throw(NoSuchPartError)
  {
    check_has_part(cur_extract_idx_);
    copy_msg(msg, parts_[cur_extract_idx_++].msg());
    return *this;
  }

  Sink&
  NullMessage(Sink& out)
  {
    Part p;
    out << p;
    return out;
  }

  Sink&
  Flush(Sink& out)
  {
    out.flush();
    return out;
  }

  template <>
  void
  Outgoing<SimpleRouting>::send_routing(
    PartsVec* routing) throw (ZmqErrorType)
  {
  }

  template <>
  void
  Outgoing<XRouting>::send_routing(
    PartsVec* routing) throw (ZmqErrorType)
  {
    if (routing == 0)
    {
      ZMQMESSAGE_LOG_STREAM <<
        "X route: route vector is empty, send null message only"
        << ZMQMESSAGE_LOG_TERM;
      add_pending_routing_part();
      Part part;
      send_one(part, false); //empty part
    }
    else
    {
      bool copy = options() & OutOptions::COPY_INCOMING;
      for (PartsVec::iterator it = routing->begin();
        it != routing->end(); ++it)
      {
        add_pending_routing_part();
        send_one(*it, copy);
      }
    }
  }

  Sink&
  Sink::operator<< (Part& msg)
    throw (ZmqErrorType)
  {
    bool copy_mode = options_ & OutOptions::COPY_INCOMING;
    bool use_copy = false;
    if (incoming_)
    {
      PartsVec::iterator it = std::find(
        incoming_->parts_.begin(), incoming_->parts_.end(), msg);
      if (it != incoming_->parts_.end())
      {
        use_copy = copy_mode;
      }
    }
    send_one(msg, use_copy);
    return *this;
  }

  Sink&
  Sink::operator<< (const RawMessage& m)
    throw (ZmqErrorType)
  {
    if (m.deleter)
    {
      Part part(m.data.ptr, m.sz, m.deleter, 0);
      send_owned(part);
    }
    else
    {
      Part part;
      init_msg(m.data.cptr, m.sz, part.msg());
      send_owned(part);
    }
    return *this;
  }

  void
  Sink::add_to_queue(Part& msg)
  {
    outgoing_queue_->parts_.push_back(msg);
  }

  void
  Sink::do_send_one(
    Part& msg, bool last)
    throw (ZmqErrorType)
  {
    int flag = 0;
    if (!last) flag |= ZMQ_SNDMORE;
    if (options_ & OutOptions::NONBLOCK) flag |= ZMQ_NOBLOCK;

    if (send_observer_ && pending_routing_parts_ == 0)
    {
      send_observer_->on_send_part(msg.msg());
    }

    ZMQMESSAGE_LOG_STREAM
      << "Outgoing sending msg, " << msg.msg().size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)msg.msg().data(),
        std::min(msg.msg().size(), static_cast<size_t>(256)))
      << ", flag = " << flag << ZMQMESSAGE_LOG_TERM;

    send_msg(dst_, msg.msg(), flag);

    if (pending_routing_parts_ > 0)
    {
      --pending_routing_parts_;
    }
  }

  bool
  Sink::try_send_first_cached(bool last) throw(ZmqErrorType)
  {
    assert(state_ == NOTSENT);
    assert(cached_.valid());
    bool ret = true;
    try
    {
      if (options_ & OutOptions::EMULATE_BLOCK_SENDS)
      {
        errno = EAGAIN;
        ZMQMESSAGE_LOG_STREAM
          << "Emulating blocking send!" << ZMQMESSAGE_LOG_TERM;
        throw_zmq_error("Emulating blocking send");
      }
      else
      {
        do_send_one(cached_, last);
        state_ = SENDING;
      }
    }
    catch (const ZmqErrorType& e)
    {
      ret = false;
      if (errno == EAGAIN)
      {
        if (options_ & OutOptions::CACHE_ON_BLOCK)
        {
          ZMQMESSAGE_LOG_STREAM
            << "Cannot send first outgoing message: would block: start caching"
            << ZMQMESSAGE_LOG_TERM;
          state_ = QUEUEING;
          outgoing_queue_.reset(new Multipart);
          add_to_queue(cached_);
        }
        else if (options_ & OutOptions::DROP_ON_BLOCK)
        {
          ZMQMESSAGE_LOG_STREAM <<
            "Cannot send first outgoing message: would block: dropping" <<
            ZMQMESSAGE_LOG_TERM;
          state_ = DROPPING;
        }
        else
        {
          throw;
        }
      }
      else
      {
        cached_.mark_invalid();
        ZMQMESSAGE_LOG_STREAM <<
          "Cannot send first outgoing message: error: " << e.what() <<
          ZMQMESSAGE_LOG_TERM;

        throw;
      }
    }
    return ret;
  }

  void
  Sink::send_one(Part& msg, bool use_copy) throw(ZmqErrorType)
  {
    if (use_copy)
    {
      Part p;
      p.copy(msg);
      send_owned(p);
    }
    else
    {
      send_owned(msg);
    }
  }

  void
  Sink::send_owned(Part& owned) throw(ZmqErrorType)
  {
    switch (state_)
    {
    case NOTSENT:
      if (!cached_.valid())
      {
        cached_.move(owned);
        break;
      }
      if (try_send_first_cached(false))
      {
        cached_.move(owned);
      }
      else
      {
        if (state_ == QUEUEING)
        {
          add_to_queue(owned);
        }
      }

      break;
    case SENDING:
      if (cached_.valid())
      {
        do_send_one(cached_, false);
        cached_.move(owned);
      }
      else
      {
        ZMQMESSAGE_LOG_STREAM <<
          "Outgoing message in state SENDING, no messages cached yet - strange"
          << ZMQMESSAGE_LOG_TERM;
        cached_.move(owned);
      }

      break;
    case QUEUEING:
      assert(outgoing_queue_.get());
      add_to_queue(owned);

      break;
    case DROPPING:
      break;
    case FLUSHED:
      ZMQMESSAGE_LOG_STREAM << "trying to send a message in FLUSHED state"
        << ZMQMESSAGE_LOG_TERM;
      break;
    }
  }

  void
  Sink::flush() throw(ZmqErrorType)
  {
    if (state_ == DROPPING)
    {
      return;
    }
    if (cached_.valid())
    {
      //handle cached
      switch (state_)
      {
      case NOTSENT:
        try_send_first_cached(true);
        break;
      case SENDING:
        do_send_one(cached_, true);
        break;

      default:
        //cannot be
        break;
      }
      cached_.mark_invalid();
    }

    if (state_ != FLUSHED)
    {
      if (send_observer_)
      {
        send_observer_->on_flush();
      }
      state_ = FLUSHED;
    }
  }

  void
  Sink::send_incoming_messages(size_t idx_from, size_t idx_to)
    throw(ZmqErrorType)
  {
    if (!incoming_)
    {
      return;
    }
    bool copy = options_ & OutOptions::COPY_INCOMING;
    send_incoming_messages(*incoming_, copy, idx_from, idx_to);
  }

  void
  Sink::send_incoming_messages(Multipart& multipart,
    bool copy, size_t idx_from, size_t idx_to)
    throw(ZmqErrorType)
  {
    const size_t to = std::min(idx_to, multipart.parts_.size());
    for (size_t i = idx_from; i < to; ++i)
    {
      send_one(multipart.parts_[i], copy);
    }
  }

  void
  Sink::relay_from(
    zmq::socket_t& relay_src, ReceiveObserver* receive_observer)
    throw (ZmqErrorType)
  {
    for (bool more = has_more(relay_src); more; )
    {
      Part cur_part;
      recv_msg(relay_src, cur_part.msg());
      more = has_more(relay_src);
      if (receive_observer)
      {
        receive_observer->on_receive_part(cur_part.msg(), more);
      }
      send_owned(cur_part);
    }
  }

  Sink::~Sink()
  {
    try
    {
      flush();
    }
    catch (const ZmqErrorType& e)
    {
      ZMQMESSAGE_LOG_STREAM <<
        "Flushing outgoing message failed: " << e.what()
        << ZMQMESSAGE_LOG_TERM;
    }
  }
}

#endif /* ZMQMESSAGE_ZMQMESSAGEFULLIMPL_HPP_ */

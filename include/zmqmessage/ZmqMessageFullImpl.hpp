/**
 * @file ZmqMessageFullImpl.hpp
 * @author askryabin
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
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock)
    throw(ZmqErrorType)
  {
    int base_flags = nonblock ? ZMQ_NOBLOCK : 0;
    for (size_t i = 0; i < multipart.size(); ++i)
    {
      int flags = base_flags | ((i < multipart.size()-1) ? ZMQ_SNDMORE : 0);
      send_msg(sock, *(multipart.parts_[i]), flags);
    }
  }

  void
  Multipart::clear()
  {
    std::for_each(
        parts_.begin(), parts_.end(),
        &Private::del_obj_not_null<zmq::message_t>
    );
    parts_.clear();
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
    if (parts_[n] == 0)
    {
      std::ostringstream ss;
      ss << "Multipart zmq message at " << n <<
          " is not owned by this multipart";
      throw NoSuchPartError(ss.str());
    }
  }

  zmq::message_t&
  Multipart::operator[](size_t i) throw (NoSuchPartError)
  {
    check_has_part(i);
    return *(parts_[i]);
  }

  zmq::message_t*
  Multipart::release(size_t i)
  {
    if (i >= size())
    {
      return 0;
    }
    zmq::message_t* p = parts_[i];
    parts_[i] = 0;
    return p;
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
      zmq::message_t* msg = new zmq::message_t;
      routing_.push_back(msg);

      recv_msg(sock, *msg);
      ZMQMESSAGE_LOG_STREAM << "Received X route: " << msg->size() << "bytes;"
        << ZMQMESSAGE_LOG_TERM;

      if (msg->size() == 0)
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

  XRouting::~XRouting()
  {
    std::for_each(
        routing_.begin(), routing_.end(),
        &Private::del_obj_not_null<zmq::message_t>
    );
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
    parts_.push_back(new zmq::message_t);
    zmq::message_t& cur_part = *(parts_.back());

    recv_msg(src_, cur_part);
    bool more = has_more(src_);

    ZMQMESSAGE_LOG_STREAM << "Incoming received "
      << cur_part.size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)cur_part.data(),
        std::min(cur_part.size(), static_cast<size_t>(256)))
      << ", has more = " << more << ZMQMESSAGE_LOG_TERM;
    return more;
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
    append_message_data(*(parts_.back()), area);
    if (is_terminal_)
    {
      return 1;
    }

    size_t delim_sz = (delimiter) ? ::strlen(delimiter) : 0;

    int num_messages = 1;
    for (; has_more(src_); ++num_messages)
    {
      if (delim_sz)
      {
        std::vector<char>::size_type sz = area.size();
        area.resize(sz + delim_sz);
        std::copy(delimiter, delimiter + delim_sz, area.begin() + sz);
      }
      zmq::message_t data_buff;
      recv_msg(src_, data_buff);
      append_message_data(data_buff, area);
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

    zmq::message_t data_buff;
    int num_messages = 0;
    if (parts_.empty()) //we haven't received anything
    {
      if (!try_recv_msg(src_, data_buff, ZMQ_NOBLOCK))
      {
        return 0;
      }
      num_messages = 1;
    }

    for (; has_more(src_); ++num_messages)
    {
      recv_msg(src_, data_buff);
    }
    return num_messages;
  }

  template <class RoutingPolicy>
  Incoming<RoutingPolicy>&
  Incoming<RoutingPolicy>::operator>> (
    zmq::message_t& msg) throw(NoSuchPartError)
  {
    check_has_part(cur_extract_idx_);
    copy_msg(msg, *(parts_[cur_extract_idx_++]));
    return *this;
  }

  Sink&
  NullMessage(Sink& out)
  {
    out << new zmq::message_t(0);
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
    MsgPtrVec* routing) throw (ZmqErrorType)
  {
  }

  template <>
  void
  Outgoing<XRouting>::send_routing(
    MsgPtrVec* routing) throw (ZmqErrorType)
  {
    if (routing == 0)
    {
      ZMQMESSAGE_LOG_STREAM <<
        "X route: route vector is empty, send null message only"
        << ZMQMESSAGE_LOG_TERM;
      send_one(new zmq::message_t, false);
    }
    else
    {
      bool copy = options() & OutOptions::COPY_INCOMING;
      for (MsgPtrVec::iterator it = routing->begin();
          it != routing->end(); ++it)
      {
        zmq::message_t* msg = *it;
        if (!copy)
        {
          *it = 0;
        }
        send_one(msg, copy);
      }
    }
  }

  Sink&
  Sink::operator<< (zmq::message_t& msg)
    throw (ZmqErrorType)
  {
    bool copy_mode = options_ & OutOptions::COPY_INCOMING;
    bool use_copy = false;
    if (incoming_)
    {
      MsgPtrVec::iterator it = std::find(
        incoming_->parts_.begin(), incoming_->parts_.end(), &msg);
      if (it != incoming_->parts_.end())
      {
        use_copy = copy_mode;
        if (!copy_mode)
        {
          *it = 0;
        }
      }
    }
    send_one(&msg, use_copy);
    return *this;
  }

  Sink&
  Sink::operator<< (const RawMessage& m)
    throw (ZmqErrorType)
  {
    if (m.deleter)
    {
      send_owned(new zmq::message_t(m.data.ptr, m.sz, m.deleter, 0));
    }
    else
    {
      MsgPtr msg(new zmq::message_t);
      init_msg(m.data.cptr, m.sz, *msg);
      send_owned(msg.release());
    }
    return *this;
  }

  void
  Sink::add_to_queue(
    zmq::message_t* msg)
  {
    outgoing_queue_->parts_.push_back(msg);
  }

  void
  Sink::do_send_one(
    zmq::message_t* msg, bool last)
    throw (ZmqErrorType)
  {
    int flag = 0;
    if (!last) flag |= ZMQ_SNDMORE;
    if (options_ & OutOptions::NONBLOCK) flag |= ZMQ_NOBLOCK;

    ZMQMESSAGE_LOG_STREAM
      << "Outgoing sending msg, " << msg->size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)msg->data(),
        std::min(msg->size(), static_cast<size_t>(256)))
      << ", flag = " << flag << ZMQMESSAGE_LOG_TERM;

    send_msg(dst_, *msg, flag);
  }

  bool
  Sink::try_send_first_cached(bool last)
    throw(ZmqErrorType)
  {
    assert(state_ == NOTSENT);
    assert(cached_.get());
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
        do_send_one(cached_.get(), last);
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
          add_to_queue(cached_.release());
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
        cached_.reset(0);
        ZMQMESSAGE_LOG_STREAM <<
          "Cannot send first outgoing message: error: " << e.what() <<
          ZMQMESSAGE_LOG_TERM;

        throw;
      }
    }
    return ret;
  }

  void
  Sink::send_one(
    zmq::message_t* msg, bool use_copy) throw(ZmqErrorType)
  {
    MsgPtr p(0);
    if (use_copy)
    {
      p.reset(new zmq::message_t);
      copy_msg(*p, *msg);
    }
    else
    {
      p.reset(msg);
    }
    send_owned(p.release());
  }

  void
  Sink::send_owned(
    zmq::message_t* owned) throw(ZmqErrorType)
  {
    MsgPtr msg(owned);
    switch (state_)
    {
    case NOTSENT:
      if (!cached_.get())
      {
        cached_ = msg;
        break;
      }
      if (try_send_first_cached(false))
      {
        cached_ = msg;
      }
      else
      {
        if (state_ == QUEUEING)
        {
          add_to_queue(msg.release());
        }
      }

      break;
    case SENDING:
      if (cached_.get())
      {
        do_send_one(cached_.get(), false);
        cached_ = msg;
      }
      else
      {
        ZMQMESSAGE_LOG_STREAM <<
          "Outgoing message in state SENDING, no messages cached yet - strange"
          << ZMQMESSAGE_LOG_TERM;
        cached_ = msg;
      }

      break;
    case QUEUEING:
      assert(outgoing_queue_.get());
      add_to_queue(msg.release());

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
    if (!cached_.get())
    {
      state_ = FLUSHED;
      return;
    }

    //handle cached
    switch (state_)
    {
    case NOTSENT:
      try_send_first_cached(true);
      break;
    case SENDING:
      do_send_one(cached_.get(), true);
      break;

    default:
      //cannot be
      break;
    }
    cached_.reset(0);
    state_ = FLUSHED;
  }

  void
  Sink::send_incoming_messages(size_t idx_from)
    throw(ZmqErrorType)
  {
    if (!incoming_)
    {
      return;
    }
    bool copy = options_ & OutOptions::COPY_INCOMING;
    for (size_t i = idx_from; i < incoming_->parts_.size(); ++i)
    {
      zmq::message_t* msg = incoming_->parts_[i];
      if (!copy)
      {
        incoming_->parts_[i] = 0;
      }
      send_one(msg, copy);
    }
  }

  void
  Sink::relay_from(zmq::socket_t& relay_src)
    throw (ZmqErrorType)
  {
    while (has_more(relay_src))
    {
      MsgPtr cur_part(new zmq::message_t);
      recv_msg(relay_src, *cur_part);
      send_owned(cur_part.release());
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

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
        send_observer->on_send_part(multipart[i].msg());
      }
      send_msg(sock, multipart[i].msg(), flags);
    }
    if (send_observer)
    {
      send_observer->on_flush();
    }
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
    if (!(*parts_ptr_)[n].valid())
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
    return (*parts_ptr_)[i];
  }

  const Part&
  Multipart::operator[](size_t i) const throw (NoSuchPartError)
  {
    check_has_part(i);
    return (*parts_ptr_)[i];
  }

  Part
  Multipart::release(size_t i)
  {
    if (i >= size())
    {
      return Part(false);
    }
    return (*parts_ptr_)[i]; //copy
  }

  void
  XRouting::log_routing_received() const
  {
    ZMQMESSAGE_LOG_STREAM << "Receiving multipart, route received: "
      << size_ << " parts";
  }

  void
  XRouting::receive_routing(zmq::socket_t& sock)
    throw (MessageFormatError, ZmqErrorType)
  {
    if (size_)
    {
      return;
    }

    for (int i = 0; ; ++i)
    {
      Part* const part = next();
      if (!part)
      {
        std::ostringstream ss;
        ss << "Receiving multipart message: reading route info failed: "
            << "part " << (i+1) << " cannot be allocated.";
        throw MessageFormatError(ss.str());
      }

      recv_msg(sock, part->msg());
      ZMQMESSAGE_LOG_STREAM <<
        "Received X route: " << part->msg().size() << "bytes;" <<
        ZMQMESSAGE_LOG_TERM;

      if (part->msg().size() == 0)
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
    Part* routing, size_t num) throw (ZmqErrorType)
  {
  }

  template <>
  void
  Outgoing<XRouting>::send_routing(
    Part* routing, size_t num) throw (ZmqErrorType)
  {
    if (routing == 0 || num == 0)
    {
      ZMQMESSAGE_LOG_STREAM <<
        "X route: route is empty, send null message only"
        << ZMQMESSAGE_LOG_TERM;
      add_pending_routing_part();
      Part part;
      send_one(part, false); //empty part
    }
    else
    {
      const bool copy = options() & OutOptions::COPY_INCOMING;
      for (size_t n = 0; n < num; ++n)
      {
        add_pending_routing_part();
        send_one(routing[n], copy);
      }
    }
  }

  Sink&
  Sink::operator<< (Part& msg)
    throw (ZmqErrorType)
  {
    const bool copy_mode = options_ & OutOptions::COPY_INCOMING;
    bool use_copy = false;
    if (incoming_)
    {
      Part* p = std::find(
        incoming_->parts(), incoming_->parts() + incoming_->size(), msg);
      if (p != incoming_->parts() + incoming_->size())
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
    Part* target = outgoing_queue_->next();
    assert(target);
    target->move(msg);
  }

  void
  Sink::do_send_one(
    Part& msg, bool last)
    throw (ZmqErrorType)
  {
    const int flags = get_send_flags(last);
    notify_on_send(msg, flags);

    send_msg(dst_, msg.msg(), flags);

    if (pending_routing_parts_ > 0)
    {
      --pending_routing_parts_;
    }
  }

  bool
  Sink::do_send_one_non_strict(
    Part& msg, bool last)
    throw (ZmqErrorType)
  {
    const int flags = get_send_flags(last);
    notify_on_send(msg, flags);

    bool ok = false;
    try
    {
      ok = dst_.send(msg, flags);
    }
    catch (const zmq::error_t& e)
    {
      throw_zmq_exception(e);
    }

    if (ok && pending_routing_parts_ > 0)
    {
      --pending_routing_parts_;
    }
    return ok;
  }

  int
  Sink::get_send_flags(bool last) const
  {
    int flag = 0;
    if (!last) flag |= ZMQ_SNDMORE;
    if (options_ & OutOptions::NONBLOCK) flag |= ZMQ_NOBLOCK;
    return flag;
  }

  void
  Sink::notify_on_send(Part& msg, int flag)
  {
    if (send_observer_ && pending_routing_parts_ == 0)
    {
      send_observer_->on_send_part(msg.msg());
    }

    ZMQMESSAGE_LOG_STREAM
      << "Outgoing sending msg, " << msg.msg().size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)msg.msg().data(),
        std::min(msg.msg().size(), static_cast<size_t>(256)))
      << ", flag = " << flag << ZMQMESSAGE_LOG_TERM;
  }

  bool
  Sink::try_send_first_cached(bool last) throw(ZmqErrorType)
  {
    assert(state_ == NOTSENT);
    assert(cached_.valid());
    bool blocked = false;

    try
    {
      if (options_ & OutOptions::EMULATE_BLOCK_SENDS)
      {
        blocked = true;
        ZMQMESSAGE_LOG_STREAM
          << "Emulating blocking send!" << ZMQMESSAGE_LOG_TERM;
      }
      else
      {
        blocked = !do_send_one_non_strict(cached_, last);
      }
    }
    catch (const ZmqErrorType& e)
    {
      cached_.mark_invalid();
      ZMQMESSAGE_LOG_STREAM <<
        "Cannot send first outgoing message: error: " << e.what() <<
        ZMQMESSAGE_LOG_TERM;
      throw;
    }

    if (blocked)
    {
      if (options_ & OutOptions::CACHE_ON_BLOCK)
      {
        ZMQMESSAGE_LOG_STREAM
          << "Cannot send first outgoing message: would block: start caching"
          << ZMQMESSAGE_LOG_TERM;
        state_ = QUEUEING;
        outgoing_queue_.reset(new QueueContainer(init_queue_len));
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
        ZMQMESSAGE_LOG_STREAM <<
          "Cannot send first outgoing message: would block: throwing error" <<
          ZMQMESSAGE_LOG_TERM;
        throw_zmq_exception(zmq::error_t());
      }
      return false;
    }

    state_ = SENDING;
    return true;
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
    const size_t to = std::min(idx_to, multipart.size());
    for (size_t i = idx_from; i < to; ++i)
    {
      send_one(multipart[i], copy);
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

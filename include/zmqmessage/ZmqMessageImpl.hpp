/**
 * @file ZmqMessageImpl.hpp
 * @author askryabin
 *
 */

#include <functional>
#include <algorithm>

#ifndef ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_
#define ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_

/////////////////////////////////////////////////
//       definition of templates
/////////////////////////////////////////////////

namespace ZmqMessage
{
  template <typename T>
  void
  Multipart::iterator<T>::set_cur()
  {
    if (end())
    {
      return;
    }
    if (messages_[idx_] == 0)
    {
      cur_ = T();
      return;
    }
    if (binary_mode_)
    {
      get_bin(*(messages_[idx_]), cur_);
    }
    else
    {
      cur_ = get<T>(*(messages_[idx_]));
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
  void
  Incoming<RoutingPolicy>::receive(
      size_t parts, const char* part_names[],
      size_t part_names_length, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType)
  {
    RoutingPolicy::receive_routing(src_);
    RoutingPolicy::log_routing_received();

    for (size_t i = 0; i < parts; ++i)
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
          "(" << i << "), expected more";
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
            "(" << i << "), expected no more messages";
          throw MessageFormatError(ss.str());
        }
      }
      if (i == parts - 1 && !more)
      {
        is_terminal_ = true;
      }
    }
  }

  template <class RoutingPolicy>
  void
  Incoming<RoutingPolicy>::receive_all(
      size_t min_parts, const char* part_names[], size_t part_names_length)
      throw (MessageFormatError, ZmqErrorType)
  {
    receive(min_parts, part_names, part_names_length, false);

    while (!is_terminal_)
    {
      is_terminal_ = !receive_one();
    }
  }

  template <class RoutingPolicy>
  void
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
      return 1;
    }

    int num_messages = 1;
    for (; has_more(src_); ++num_messages)
    {
      zmq::message_t data_buff;
      recv_msg(src_, data_buff);
    }
    return num_messages;
  }

  template <class RoutingPolicy>
  template <typename T>
  Incoming<RoutingPolicy>&
  Incoming<RoutingPolicy>::operator>> (T& t) throw(NoSuchPartError)
  {
    check_has_part(cur_extract_idx_);
    if (binary_mode_)
    {
      get_bin(*(parts_[cur_extract_idx_++]), t);
    }
    else
    {
      t = get<T>(*(parts_[cur_extract_idx_++]));
    }
    return *this;
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

  template <class RoutingPolicy>
  template <typename T>
  Outgoing<RoutingPolicy>&
  Outgoing<RoutingPolicy>::operator<< (const T& t)
  {
    if (options_ & OutOptions::BINARY_MODE)
    {
      return operator<<(RawMessage(&t, sizeof(T)));
    }
    else
    {
      MsgPtr msg(new zmq::message_t);
      init_msg(t, *msg);
      send_owned(msg.release());
    }
    return *this;
  }

  template <class RoutingPolicy>
  Outgoing<RoutingPolicy>&
  Outgoing<RoutingPolicy>::operator<< (zmq::message_t& msg)
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

  template <class RoutingPolicy>
  Outgoing<RoutingPolicy>&
  Outgoing<RoutingPolicy>::operator<< (const RawMessage& m)
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::add_to_queue(
    zmq::message_t* msg)
  {
    outgoing_queue_->parts_.push_back(msg);
  }

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::do_send_one(
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

  template <class RoutingPolicy>
  bool
  Outgoing<RoutingPolicy>::try_send_first_cached(bool last)
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::send_one(
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::send_owned(
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
        add_to_queue(msg.release());
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::flush() throw(ZmqErrorType)
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::send_incoming_messages(size_t idx_from)
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

  template <class RoutingPolicy>
  void
  Outgoing<RoutingPolicy>::relay_from(zmq::socket_t& relay_src)
    throw (ZmqErrorType)
  {
    while (has_more(relay_src))
    {
      MsgPtr cur_part(new zmq::message_t);
      recv_msg(relay_src, *cur_part);
      send_owned(cur_part.release());
    }
  }

  template <class RoutingPolicy>
  template <class OccupationAccumulator>
  void
  Outgoing<RoutingPolicy>::relay_from(
    zmq::socket_t& relay_src, OccupationAccumulator acc)
    throw (ZmqErrorType)
  {
    while (has_more(relay_src))
    {
      MsgPtr cur_part(new zmq::message_t);
      recv_msg(relay_src, *cur_part);
      size_t sz = cur_part->size();
      acc(sz);
      send_owned(cur_part.release());
    }
  }

  template <class RoutingPolicy>
  Outgoing<RoutingPolicy>::~Outgoing()
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

#endif /* ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_ */

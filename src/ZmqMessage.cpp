/**
 * @file ZmqMessage.cpp
 * @author askryabin
 *
 */

#include "ZmqMessage.hpp"
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
  XRouting::receive_routing(zmq::socket_t& sock)
    throw (MessageFormatError, ZmqErrorType)
  {
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
      bool copy = options_ & OutOptions::COPY_INCOMING;
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
}

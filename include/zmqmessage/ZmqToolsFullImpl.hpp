/**
 * @file ZmqToolsFullImpl.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Full definition of all ZmqMessage tools functions.
 * In HEADERONLY builds, they are included in client code and inlined.
 * They are included either in library code,
 * so in non-headeronly builds this code does not appear in client modules.
 */

#ifndef ZMQMESSAGE_ZMQTOOLSFULLIMPL_HPP_
#define ZMQMESSAGE_ZMQTOOLSFULLIMPL_HPP_

#include <string>

namespace ZmqMessage
{
  time_t
  get_time(zmq::message_t& message)
  {
    errno = 0;
    std::string str;
    get(message, str);
    time_t tm = strtol(str.data(), 0, 10);
    if (errno)
    {
      return time_t();
    }
    return tm;
  }

  void
  init_msg(const void* t, size_t sz, zmq::message_t& msg)
  {
    try
    {
      msg.rebuild(sz);
      ::memcpy(msg.data(), t, sz);
    }
    catch (const zmq::error_t& e)
    {
      throw_zmq_exception(e);
    }
  }

  bool
  has_more(zmq::socket_t& sock)
  {
    int64_t more = 0;
    size_t more_size = sizeof(more);
    sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    return (more != 0);
  }

  int
  relay_raw(zmq::socket_t& src, zmq::socket_t& dst, bool check_first_part)
  {
    int relayed = 0;
    for (
      bool more = check_first_part ? has_more(src) : true;
      more; ++relayed)
    {
      zmq::message_t cur_part;
      src.recv(&cur_part);
      more = has_more(src);
      int flag = more ? ZMQ_SNDMORE : 0;
      send_msg(dst, cur_part, flag);
    }
    return relayed;
  }
}

#endif /* ZMQMESSAGE_ZMQTOOLSFULLIMPL_HPP_ */

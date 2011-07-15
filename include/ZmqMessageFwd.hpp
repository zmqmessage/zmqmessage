/**
 * @file ZmqMessageFwd.hpp
 * @author askryabin
 * Forward declarations of ZmqMessage classes.
 */

#include <zmq.hpp>
#include <zmqmessage/Config.hpp>

#ifndef ZMQMESSAGE_ZMQMESSAGEFWD_HPP_
#define ZMQMESSAGE_ZMQMESSAGEFWD_HPP_

namespace ZmqMessage
{
  /**
   * @class MessageFormatError
   * @brief
   * Thrown when received multipart message consists of wrong number of parts.
   */
  ZMQMESSAGE_EXCEPTION_MACRO(MessageFormatError)
  ;

  /**
   * @class NoSuchPartError
   * @brief
   * Thrown when trying to access inexistent part in received message
   */
  ZMQMESSAGE_EXCEPTION_MACRO(NoSuchPartError)
  ;

  class SimpleRouting;

  class XRouting;

  class Multipart;

  template <class RoutingPolicy>
  class Incoming;

  template <class RoutingPolicy>
  class Outgoing;

  /**
   * Send given message to destination socket
   */
  ZMQMESSAGE_HEADERONLY_INLINE
  void
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock)
    throw(ZmqErrorType);

}

#endif /* ZMQMESSAGE_ZMQMESSAGEFWD_HPP_ */

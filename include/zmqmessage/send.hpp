/**
 * @file send.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_SEND_HPP_
#define ZMQMESSAGE_SEND_HPP_

#include <ZmqMessageFwd.hpp>
#include <zmqmessage/exceptions.hpp>

namespace ZmqMessage
{
  /**
   * Send given message to destination socket
   */
  void
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock,
    SendObserver* send_observer = 0)
    throw(ZmqErrorType);
}

#endif /* ZMQMESSAGE_SEND_HPP_ */

/**
 * @file Routing.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_ROUTING_HPP_
#define ZMQMESSAGE_ROUTING_HPP_

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/NonCopyable.hpp>
#include <zmqmessage/PartsStorage.hpp>

namespace ZmqMessage
{
  /**
   * @brief Simple (not-X) Routing policy
   *
   * Routing policy for sending/receiving message through "not-X" ZMQ endpoints
   * (socket types PUSH, PULL, REQ, REP, PUB, SUB,...)
   */
  class SimpleRouting
  {
  protected:
    inline void
    receive_routing(zmq::socket_t& sock) {}

    inline
    Part*
    get_routing() const
    {
      return 0;
    }

    inline
    size_t
    get_routing_num() const
    {
      return 0;
    }

    inline
    void
    log_routing_received() const {}
  };

  /**
   * @brief X routing policy
   *
   * Routing policy for sending/receiving message through "X" ZMQ endpoints
   * (socket types XREQ and XREP)
   */

  class XRouting : private ZMQMESSAGE_ROUTING_STORAGE
  {
  protected:
    XRouting() : ZMQMESSAGE_ROUTING_STORAGE(ZMQMESSAGE_ROUTING_CAPACITY)
    {}

    void
    receive_routing(zmq::socket_t& sock)
    throw (MessageFormatError, ZmqErrorType);

    inline
    Part*
    get_routing()
    {
      return parts_;
    }

    inline
    size_t
    get_routing_num() const
    {
      return size_;
    }

    void
    log_routing_received() const;
  };
}

#endif /* ZMQMESSAGE_ROUTING_HPP_ */

/**
 * @file Outgoing.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_OUTGOING_HPP_
#define ZMQMESSAGE_OUTGOING_HPP_

#include <ZmqMessageFwd.hpp>
#include <zmqmessage/OutOptions.hpp>
#include <zmqmessage/Sink.hpp>

namespace ZmqMessage
{
  /**
   * @brief Represents outgoing message to be sent.
   *
   * All message parts are either sent (if possible)
   * OR become exclusively owned by this object,
   * if sending of parts immediately would block
   * and Outgoing created with
   * OutOptions.NONBLOCK and OutOptions.CACHE_ON_BLOCK.
   * That Multipart is detachable
   * (ownership may be yielded, see @c detach() method).
   * Outgoing message may be linked with corresponding incoming message
   * (when @c Incoming reference given to constructor).
   * In this case the ownership of message parts may be transferred
   * from incoming to outgoing message (by @c operator << on message_t),
   * to avoid copying.
   */
  template <class RoutingPolicy>
  class ZMQMESSAGE_DLL_PUBLIC Outgoing : public Sink
  {
  private:

    void
    send_routing(Part* routing, size_t num) throw(ZmqErrorType);

  public:

    using Sink::iterator;

    /**
     * Routing policy used for sending multipart message
     */
    typedef RoutingPolicy RoutingPolicyType;

    Outgoing(zmq::socket_t& dst, unsigned options) :
      Sink(dst, options)
    {
      send_routing(0, 0);
    }

    explicit
    Outgoing(OutOptions out_opts) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer)
    {
      send_routing(0, 0);
    }

    /**
     * Outgoing message is a response to the given Incoming message,
     * so we resend Incoming's routing first.
     */
    template <typename InRoutingPolicy, typename InPartsStorage>
    Outgoing(zmq::socket_t& dst,
      Incoming<InRoutingPolicy, InPartsStorage>& incoming,
      unsigned options) throw(ZmqErrorType) :
      Sink(dst, options, 0, &incoming)
    {
      send_routing(incoming.get_routing(), incoming.get_routing_num());
    }

    /**
     * Outgoing message is a response to the given Incoming message,
     * so we resend Incoming's routing first.
     */
    template <typename InRoutingPolicy, typename InPartsStorage>
    Outgoing(OutOptions out_opts,
      Incoming<InRoutingPolicy, InPartsStorage>& incoming)
      throw(ZmqErrorType) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer, &incoming)
    {
      send_routing(incoming.get_routing(), incoming.get_routing_num());
    }

    /**
     * Outgoing message is NOT a response to the given Incoming message,
     * so we send normal routing.
     */
    Outgoing(zmq::socket_t& dst, Multipart& incoming,
      unsigned options) throw(ZmqErrorType) :
      Sink(dst, options, 0, &incoming)
    {
      send_routing(0, 0);
    }

    /**
     * Outgoing message is NOT a response to the given Incoming message,
     * so we send normal routing.
     */
    Outgoing(OutOptions out_opts, Multipart& incoming) throw(ZmqErrorType) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer, &incoming)
    {
      send_routing(0, 0);
    }
  };
}

#endif /* ZMQMESSAGE_OUTGOING_HPP_ */

/**
 * @file Manip.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_MANIP_HPP_
#define ZMQMESSAGE_MANIP_HPP_

#include <ZmqMessageFwd.hpp>

namespace ZmqMessage
{
  /**
   * @brief Null (empty) message marker to send
   *
   * When passed to @c Outgoing (operator <<)
   * creates null (empty) message part
   */
  Sink&
  NullMessage(Sink& out);

  /**
   * @brief Skip current message part in incoming message
   *
   * When passed to @c Incoming (operator >>)
   * just skips current message and moves receive pointer to next one.
   * Also, usual checking that current part exists is performed.
   */
  template<typename RoutingPolicy, typename PartsStorage>
  Incoming<RoutingPolicy, PartsStorage>&
  Skip(Incoming<RoutingPolicy, PartsStorage>& in);

  /**
   * @brief Manipulator to flush outgoing message.
   *
   * Manipulator to finally flush (send/enqueue) the outgoing message.
   */
  Sink&
  Flush(Sink& out);

  /**
   * @brief Manipulator to switch to binary mode
   *
   * Manipulator to turn off conversion to/from text sending/receiving
   * See \ref zm_modes "modes"
   */
  template<typename StreamAlike>
  StreamAlike&
  Binary(StreamAlike& out)
  {
    out.set_binary();
    return out;
  }

  /**
   * @brief Manipulator to switch to text mode (default)
   *
   * Manipulator to turn on conversion to/from text sending/receiving.
   * See \ref zm_modes "modes"
   */
  template<typename StreamAlike>
  StreamAlike&
  Text(StreamAlike& out)
  {
    out.set_text();
    return out;
  }
}

#endif /* ZMQMESSAGE_MANIP_HPP_ */

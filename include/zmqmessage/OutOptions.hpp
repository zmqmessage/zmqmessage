/**
 * @file OutOptions.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_OUTOPTIONS_HPP_
#define ZMQMESSAGE_OUTOPTIONS_HPP_

#include <zmq.hpp>
#include <ZmqMessageFwd.hpp>

namespace ZmqMessage
{
  /**
   * @brief Options for outgoing message
   *
   * This struct holds both reference to socket and options.
   * Also it can hold SendObserver pointer to be set to Outgoing object.
   * Needed for convenient creation of @c Outgoing object.
   */
  struct OutOptions
  {
    /**
     * send nonblockingly
     */
    static const unsigned NONBLOCK = 0x1;
    /**
     * cache messages on would-block sends
     */
    static const unsigned CACHE_ON_BLOCK = 0x2;
    /**
     * drop messages on would-block sends instead of rethrowing exceptions
     */
    static const unsigned DROP_ON_BLOCK = 0x4;
    /**
     * All messages (inc. routing) taken from incoming are copied
     */
    static const unsigned COPY_INCOMING = 0x8;

    /**
     * Debug use only. Emulate blocking on all sends!
     */
    static const unsigned EMULATE_BLOCK_SENDS = 0x10;

    /**
     * Create Outgoing with insertion stream in binary mode.
     * Stream mode is maintained by Binary/Text manipulators.
     * See \ref zm_modes "modes"
     */
    static const unsigned BINARY_MODE = 0x20;

    zmq::socket_t& sock;
    unsigned options;

    typedef SendObserver* SendObserverPtr;

    SendObserverPtr send_observer;

    /**
     * Create OutOptions.
     * Note, that OutOptions doesn't take ownership on SendObserver.
     */
    inline
    OutOptions(
      zmq::socket_t& sock_p, unsigned options_p, SendObserverPtr so = 0) :
      sock(sock_p), options(options_p), send_observer(so)
    {}
  };
}

#endif /* ZMQMESSAGE_OUTOPTIONS_HPP_ */

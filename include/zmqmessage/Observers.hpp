/**
 * @file Observers.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_OBSERVERS_HPP_
#define ZMQMESSAGE_OBSERVERS_HPP_

namespace ZmqMessage
{
  /**
   * @brief Observer of incoming message parts being received.
   *
   * Incoming class may be parameterized with an object inherited from
   * SendObserver.
   * Observer will be notified when message parts are being received
   * and when the last message part is received.
   */
  class ReceiveObserver
  {
  public:
    /**
     * Next message part (excluding routing parts) has been received.
     */
    virtual
    void
    on_receive_part(zmq::message_t& msg, bool has_more) = 0;

  protected:
    virtual
    ~ReceiveObserver() {}
  };

  /**
   * @brief Observer of outgoing message parts being sent.
   *
   * Sink class may be parameterized with an object inherited from
   * SendObserver.
   * Observer will be notified when message parts are being sent
   * and when the entire Sink has been flushed.
   */
  class SendObserver
  {
  public:
    /**
     * Next message part (excluding routing parts) is about to be sent.
     */
    virtual
    void
    on_send_part(zmq::message_t& msg) = 0;

    /**
     * Sink has been flushed.
     * @param send_successful If true, all parts are actually sent.
     * If false, we couldn't send message, it's dropped.
     */
    virtual
    void
    on_flush(bool send_successful) = 0;

  protected:
    virtual
    ~SendObserver() {}
  };
}

#endif /* ZMQMESSAGE_OBSERVERS_HPP_ */
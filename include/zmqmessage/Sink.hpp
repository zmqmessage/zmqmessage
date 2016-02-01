/**
 * @file Sink.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_SINK_HPP_
#define ZMQMESSAGE_SINK_HPP_

#include <memory>
#include <climits>

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/Config.hpp>
#include <zmqmessage/NonCopyable.hpp>
#include <zmqmessage/Part.hpp>
#include <zmqmessage/Observers.hpp>
#include <zmqmessage/OutOptions.hpp>
#include <zmqmessage/MultipartContainer.hpp>
#include <zmqmessage/RawMessage.hpp>

namespace ZmqMessage
{
  /**
   * @brief Base class for Outgoing message,
   * does not depend on Routing policy.
   *
   * After Outgoing message is created, it's functionality
   * (append and send messages, caching, etc.) does not depend
   * on RoutingPolicy template parameter,
   * so it may be referenced as Sink class.
   */
  class ZMQMESSAGE_DLL_PUBLIC Sink : private Private::NonCopyable
  {
  public:
    /**
     * @brief Output iterator to facilitate inserting of message parts
     * into outgoing message.
     */
    template <typename T>
    class ZMQMESSAGE_DLL_PUBLIC iterator
    {
    public:
      typedef T value_type;
      typedef T& reference;
      typedef T* pointer;
      typedef std::ptrdiff_t difference_type;
      typedef std::output_iterator_tag iterator_category;

    private:
      class ZMQMESSAGE_DLL_LOCAL AssignProxy
      {
      public:
        AssignProxy(Sink& outgoing)
        : outgoing_(outgoing)
        {
        }

        void
        operator=(const T& output_object)
        {
          outgoing_ << output_object;
        }

      private:
        Sink& outgoing_;
      };

    public:
      explicit
      iterator(Sink& outgoing)
      : outgoing_(outgoing)
      {
      }

      iterator
      operator++()
      {
        return *this;
      }

      iterator
      operator++(int)
      {
        return *this;
      }

      AssignProxy
      operator*()
      {
        return AssignProxy(outgoing_);
      }

      bool
      operator==(const iterator<T>& rhs)
      {
        return equal(rhs);
      }

      bool
      operator!=(const iterator<T>& rhs)
      {
        return !equal(rhs);
      }

    private:
      Sink& outgoing_;

      ZMQMESSAGE_DLL_LOCAL
      bool
      equal(const iterator<T>& rhs) const
      {
        return false;
      }
    };

  private:
    typedef std::auto_ptr<zmq::message_t> MsgPtr;

    zmq::socket_t& dst_;

    unsigned options_;

    OutOptions::SendObserverPtr send_observer_;

    /**
     * If not null, the routing will be taken from it.
     * Either, when sending/inserting zmq messages to Outgoing,
     * the ownership will be disowned from incoming_.
     */
    Multipart* incoming_;

    typedef Private::MultipartContainer<DynamicPartsStorage<> > QueueContainer;

    std::auto_ptr<QueueContainer> outgoing_queue_;

    static const size_t init_queue_len = 10;

    /**
     * Keep message part until next part or flush
     *  to determine if the part is the latest in the message
     */
    Part cached_;

    enum State
    {
      NOTSENT, //<! we haven't sent messages
      SENDING, //<! we have sent at least one message, it's OK
      QUEUEING,  //<! we failed to send, queueing to @c outgoing_queue_ instead
      DROPPING,  //<! we failed to send, dropped. Terminal state
      FLUSHED  //<! Message is flushed, all messages are sent or queued, no more messages accepted
    };

    State state_;

    size_t pending_routing_parts_;

  protected:
    Sink(zmq::socket_t& dst, unsigned options,
      OutOptions::SendObserverPtr so = 0, Multipart* incoming = 0) :
      dst_(dst), options_(options), send_observer_(so), incoming_(incoming),
      outgoing_queue_(0), cached_(false), state_(NOTSENT),
      pending_routing_parts_(0)
    {}

    inline
    void
    add_pending_routing_part()
    {
      ++pending_routing_parts_;
    }

    void
    send_one(
      Part& msg, bool use_copy = false)
      throw(ZmqErrorType);

  private:
    /**
     * Default visibility here cause it's called from template operator <<
     * which may be generated within external binary.
     */
    void
    send_owned(Part& owned) throw(ZmqErrorType);

    ZMQMESSAGE_DLL_LOCAL
    void
    do_send_one(Part& msg, bool last) throw(ZmqErrorType);

    ZMQMESSAGE_DLL_LOCAL
    bool
    do_send_one_non_strict(Part& msg, bool last) throw (ZmqErrorType);

    ZMQMESSAGE_DLL_LOCAL
    inline
    int
    get_send_flags(bool last) const;

    ZMQMESSAGE_DLL_LOCAL
    void
    notify_on_send(Part& msg, int flag);

    ZMQMESSAGE_DLL_LOCAL
    bool
    try_send_first_cached(bool last) throw(ZmqErrorType);

    ZMQMESSAGE_DLL_LOCAL
    void
    add_to_queue(Part& part);

  public:
    virtual
    ~Sink();

    inline
    unsigned
    options() const
    {
      return options_;
    }

    template <typename T>
    iterator<T>
    inserter()
    {
      return iterator<T>(*this);
    }

    /**
     * Assign a pointer to SendObserver object.
     * Note, that the ownership on the given object is taken by Sink object.
     */
    inline
    void
    set_send_observer(SendObserver* se)
    {
      send_observer_ = se;
    }

    /**
     * Get pointer to incoming message this outgoing message is linked to.
     * @return null if not linked.
     */
    const Multipart*
    incoming() const
    {
      return incoming_;
    }

    /**
     * Detach heap-allocated outgoing queue (Multipart message).
     * This object's outgoing_queue_ is set to 0.
     */
    inline
    Multipart*
    detach()
    {
      return outgoing_queue_.release();
    }

    /**
     * @return if we have enqueued message parts in outgoing queue.
     */
    inline
    bool
    is_queued() const
    {
      return (outgoing_queue_.get() != 0);
    }

    /**
     * Immediate send has failed (blocking)
     * and we are dropping inserted messages
     */
    inline
    bool
    is_dropping() const
    {
      return (state_ == DROPPING);
    }

    /**
     * @return destination socket
     */
    inline
    zmq::socket_t&
    dst()
    {
      return dst_;
    }

    /**
     * Finally send or enqueue pending (cached) messages if any
     */
    void
    flush() throw(ZmqErrorType);

    void
    set_binary()
    {
      options_ |= OutOptions::BINARY_MODE;
    }

    void
    set_text()
    {
      options_ &= ~OutOptions::BINARY_MODE;
    }

    /**
     * Send all messages contained in @c incoming_ starting from idx_from
     * ending idx_to - 1
     */
    void
    send_incoming_messages(size_t idx_from = 0, size_t idx_to = UINT_MAX)
    throw(ZmqErrorType);

    /**
     * Send all messages contained in given incoming starting from idx_from
     * ending idx_to - 1
     */
    void
    send_incoming_messages(Multipart& multipart,
      bool copy, size_t idx_from = 0, size_t idx_to = UINT_MAX)
    throw(ZmqErrorType);

    /**
     * Receive and send/enqueue pending messages from relay_src socket
     */
    void
    relay_from(zmq::socket_t& relay_src,
      ReceiveObserver* receive_observer = 0) throw(ZmqErrorType);

    /**
     * Receive and send/enqueue pending messages from relay_src socket,
     * counting sizes of received messages
     * @tparam OccupationAccumulator unary functor accepting size_t
     */
    template <class OccupationAccumulator>
    void
    relay_from(
      zmq::socket_t& relay_src, OccupationAccumulator acc,
      ReceiveObserver* receive_observer)
    throw (ZmqErrorType);

    /**
     * Lowest priority in overload. Uses @c ZmqMessage::init_msg functions
     * to compose message part and send.
     */
    template <typename T>
    Sink&
    operator<< (const T& t) throw (ZmqErrorType);

    /**
     * Note, we take ownership on message
     * (unless message is part of incoming_ and COPY_INCOMING option is set).
     * We invalidate incoming_'s ownership on this message
     * (if COPY_INCOMING option is NOT set).
     * Do not pass stack-allocated messages
     * if this object is planned to be detached and used beyond current block.
     */
    Sink&
    operator<< (Part& msg) throw (ZmqErrorType);

    /**
     * Either, we take ownership on message.
     * Not checking if this message is part of incoming_.
     * If ptr contains 0, null message is sent
     */
    inline Sink&
    operator<< (MsgPtr msg) throw (ZmqErrorType)
    {
      Part part;
      if (msg.get())
      {
        part.move(*msg);
      }
      send_owned(part);
      return *this;
    }

#ifdef ZMQMESSAGE_CPP11
    /**
     * Either, we take ownership on message.
     * Not checking if this message is part of incoming_.
     * If ptr contains 0, null message is sent
     */
    inline Sink&
    operator<< (std::unique_ptr<zmq::message_t>&& msg) throw (ZmqErrorType)
    {
      Part part;
      if (msg.get())
      {
        part.move(*msg);
      }
      send_owned(part);
      return *this;
    }
#endif

    /**
     * Insert raw message (see @c RawMessage)
     */
    Sink&
    operator<< (const RawMessage& m) throw (ZmqErrorType);

    /**
     * Handle a manipulator
     */
    inline Sink&
    operator<< (Sink& (*f)(Sink&))
    {
      return f(*this);
    }
  };
}

#endif /* ZMQMESSAGE_SINK_HPP_ */

/**
 * @file ZmqMessage.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 */

#include <cstddef>
#include <vector>
#include <tr1/array>
#include <string>
#include <memory>
#include <zmq.hpp>

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/Config.hpp>
#include <zmqmessage/Observers.hpp>
#include <zmqmessage/DelObj.hpp>
#include <zmqmessage/NonCopyable.hpp>
#include <zmqmessage/MsgPtrVec.hpp>
#include <zmqmessage/RawMessage.hpp>

#include <ZmqTools.hpp>

#ifndef ZMQMESSAGE_HPP_
#define ZMQMESSAGE_HPP_

/**
 * @namespace ZmqMessage
 * @brief Main namespace for ZmqMessage library
 */
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

  /**
   * Send given message to destination socket
   */
  void
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock,
    SendObserver* send_observer = 0)
    throw(ZmqErrorType);


  // routing policies

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
    MsgPtrVec*
    get_routing()
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
  class XRouting
  {
  private:
    MsgPtrVec routing_; //!< including null message

  protected:
    void
    receive_routing(zmq::socket_t& sock)
    throw (MessageFormatError, ZmqErrorType);

    inline
    MsgPtrVec*
    get_routing()
    {
      return &routing_;
    }

    void
    log_routing_received() const;

    ~XRouting();
  };

  /**
   * @brief Basic holder of message parts.
   */
  class Multipart : private Private::NonCopyable
  {
  protected:
    MsgPtrVec parts_;

    void
    check_has_part(size_t n) const throw(NoSuchPartError);

  private:

    void
    clear();

    friend class Sink;

    friend void
    send(zmq::socket_t&, Multipart&, bool, SendObserver*)
    throw(ZmqErrorType);

  public:
    /**
     * @brief Input iterator to iterate over message parts
     */
    template <typename T>
    class iterator
    {
    public:
      typedef T value_type;
      typedef T& reference;
      typedef T* pointer;
      typedef std::ptrdiff_t difference_type;
      typedef std::input_iterator_tag iterator_category;

    private:
      explicit
      iterator(const MsgPtrVec& messages, bool binary_mode, bool end = false) :
      messages_(messages), idx_(end ? messages.size() : 0),
      binary_mode_(binary_mode)
      {
        set_cur();
      }

      iterator(const MsgPtrVec& messages, size_t idx, bool binary_mode) :
        messages_(messages), idx_(idx), binary_mode_(binary_mode)
      {
        set_cur();
      }

      friend class Multipart;

    public:
      iterator
      operator++()
      {
        ++idx_;
        set_cur();
        return *this;
      }

      iterator
      operator++(int)
      {
        iterator<T> ret_val(*this);
        ++(*this);
        return ret_val;
      }

      const T
      operator*()
      {
        return cur_;
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
      const MsgPtrVec& messages_;
      size_t idx_;
      T cur_;
      const bool binary_mode_;

      inline
      bool
      end() const
      {
        return idx_ >= messages_.size();
      }

      bool
      equal(const iterator<T>& rhs) const
      {
        return end() && rhs.end();
      }

      void
      set_cur();
    };
  public:
    virtual ~Multipart()
    {
      clear();
    }

    /**
     * Detach heap-allocated Multipart message
     * owning all the parts of this multipart.
     * After this operation, current object owns no message parts.
     */
    Multipart*
    detach();

    /**
     * Does this multipart message has a part at given index (and owns it)
     */
    inline
    bool
    has_part(size_t idx)
    {
      return (parts_.size() > idx && parts_[idx] != 0);
    }

    /**
     * Reserve enough memory for given number of message parts
     */
    inline
    void
    reserve(size_t sz)
    {
      parts_.reserve(sz);
    }

    /**
     * Obtain iterator that yields values of type T.
     * @param binary_mode: how to convert message part to T:
     * interprete as string data (if binary_mode is false)
     * or interpret as binary data (if true).
     * @tparam T must be explicitly specified:
     * @code
     * it = multipart.begin<std::string>();
     * @endcode
     */
    template <typename T>
    inline
    iterator<T>
    begin(bool binary_mode = false) const
    {
      return iterator<T>(parts_, binary_mode);
    }

    /**
     * Obtain iterator that yields values of default string type.
     */
    inline
    iterator<ZMQMESSAGE_STRING_CLASS>
    beginstr() const
    {
      return begin<ZMQMESSAGE_STRING_CLASS>();
    }

    /**
     * Obtain iterator positioned to message part at specified index
     * @param pos index, counts from 0.
     * @param binary_mode: how to convert message part to non-string type:
     * interprete as string data (if binary_mode is false)
     * or interpret as binary data (if true).
     */
    template <typename T>
    inline
    iterator<T>
    iter_at(size_t pos, bool binary_mode = false) const
    {
      return iterator<T>(parts_, pos, binary_mode);
    }

    /**
     * End iterator that yields values of type T.
     */
    template <typename T>
    inline
    iterator<T>
    end() const
    {
      return iterator<T>(parts_, false, true);
    }

    /**
     * End iterator that yields values of default string type.
     */
    inline
    iterator<ZMQMESSAGE_STRING_CLASS>
    endstr() const
    {
      return end<ZMQMESSAGE_STRING_CLASS>();
    }

    /**
     * @return number of parts in this multipart
     */
    inline
    size_t
    size() const
    {
      return parts_.size();
    }

    /**
     * Get reference to zmq::message_t by index
     */
    zmq::message_t&
    operator[](size_t i) throw (NoSuchPartError);

    /**
     * Release (disown) message at specified index.
     * @return 0 if such message is not owned by Multipart
     */
    zmq::message_t*
    release(size_t i);

    inline
    std::auto_ptr<zmq::message_t>
    release_ptr(size_t i)
    {
      return std::auto_ptr<zmq::message_t>(release(i));
    }
  };

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
  template<typename RoutingPolicy>
  Incoming<RoutingPolicy>&
  Skip(Incoming<RoutingPolicy>& in)
  {
    in.check_has_part(in.cur_extract_idx_);
    ++in.cur_extract_idx_;
    return in;
  }

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

  /**
   * @brief Incoming multipart ZMQ message.
   *
   * @tparam RoutingPolicy either SimpleRouting or XRouting -
   * rules for receiving routing info.
   */
  template <class RoutingPolicy>
  class Incoming :
  public Multipart, private RoutingPolicy
  {
  private:

    zmq::socket_t& src_; //!< source socket to receive parts from
    bool is_terminal_; //!< no more parts at end
    size_t cur_extract_idx_;
    bool binary_mode_; //!< stream flag to handle conversion

    ReceiveObserver* receive_observer_;

    void
    append_message_data(
      zmq::message_t& message, std::vector<char>& area) const;

    /**
     * Fetches one message from src_, appends message to parts_.
     * @return if we have more messages on socket
     */
    bool
    receive_one() throw(ZmqErrorType);

    bool
    do_receive_msg(zmq::message_t& msg) throw(ZmqErrorType)
    {
      recv_msg(src_, msg);
      bool more = has_more(src_);
      if (receive_observer_)
      {
        receive_observer_->on_receive_part(msg, more);
      }
      return more;
    }

    template <class OutRoutingPolicy>
    friend class Outgoing;

    inline
    MsgPtrVec*
    get_routing()
    {
      return RoutingPolicy::get_routing();
    }

    friend
    Incoming<RoutingPolicy>&
    Skip<RoutingPolicy>(Incoming<RoutingPolicy>&);

  public:
    typedef RoutingPolicy RoutingPolicyType;

    explicit Incoming(zmq::socket_t& sock)
    : src_(sock), is_terminal_(false),
      cur_extract_idx_(0), binary_mode_(false),
      receive_observer_(0)
    {
    }

    /**
     * Assign a pointer to ReceiveObserver object.
     * Note, that the Incoming does not take ownership on the given object.
     */
    inline
    void
    set_receive_observer(ReceiveObserver* observer)
    {
      receive_observer_ = observer;
    }

    /**
     * @return zmq socket to receive message parts from
     */
    inline
    zmq::socket_t&
    src()
    {
      return src_;
    }

    /**
     * @return true if we have detected, that no more message parts are
     * accessible on socket (all parts are received).
     */
    inline
    bool
    is_terminal() const
    {
      return is_terminal_;
    }

    /**
     * If received message is not already terminal (has no pending parts),
     * throws MessageFormatError exception.
     */
    void
    check_is_terminal() const throw(MessageFormatError);

    /**
     * Validate that message contains definite number of message parts.
     * @param part_names array of parts names
     * for error reporting and debug purposes
     * @param part_names_length length of part_names array
     * @param strict if true, message MUST contain exactly
     * @c part_names_length parts. Otherwise it MUST contain at least
     * @c part_names_length parts.
     */
    void
    validate(
      const char* part_names[],
      size_t part_names_length, bool strict)
    throw (MessageFormatError);

    /**
     * Receive definite number of message parts.
     * @param parts number of parts to receive
     * @param part_names array of parts names
     * for error reporting and debug purposes
     * @param part_names_length length of part_names array
     * @param check_terminal if true, message may contain not more then
     * @c parts number of parts.
     */
    Incoming <RoutingPolicy>&
    receive(
      size_t parts, const char* part_names[],
      size_t part_names_length, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     */
    template <size_t N>
    inline
    Incoming <RoutingPolicy>&
    receive(
      std::tr1::array<const char*, N> part_names, bool check_terminal)
      throw (MessageFormatError)
      {
      return receive(N, part_names.data(), check_terminal);
      }

    /**
     * @overload
     * Part names are omitted
     */
    inline
    Incoming <RoutingPolicy>&
    receive(size_t parts, bool check_terminal)
    throw (MessageFormatError, ZmqErrorType)
    {
      return receive(parts, 0, 0, check_terminal);
    }

    /**
     * @overload
     * assume size of part_names == parts
     */
    inline
    Incoming <RoutingPolicy>&
    receive(
      size_t parts, const char* part_names[], bool check_terminal)
      throw (MessageFormatError, ZmqErrorType)
      {
      return receive(parts, part_names, parts, check_terminal);
      }

    /**
     * Receives ALL messages available on socket.
     * (min @c min_parts, max unbounded).
     * Multipart message will be considered terminal.
     */
    Incoming <RoutingPolicy>&
    receive_all(
      const size_t min_parts,
      const char* part_names[], size_t part_names_length)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     * assume size of part_names == min_parts
     */
    inline
    Incoming <RoutingPolicy>&
    receive_all(const size_t min_parts, const char* part_names[])
    throw (MessageFormatError, ZmqErrorType)
    {
      return receive_all(min_parts, part_names, min_parts);
    }

    /**
     * @overload
     * No minimum parts limit (may be 0)
     */
    inline
    Incoming <RoutingPolicy>&
    receive_all()
    throw (MessageFormatError, ZmqErrorType)
    {
      return receive_all(0, 0, 0);
    }

    /**
     * @overload
     * Part names are unknown.
     */
    inline
    Incoming <RoutingPolicy>&
    receive_all(const size_t min_parts)
    throw (MessageFormatError, ZmqErrorType)
    {
      return receive_all(min_parts, 0, 0);
    }

    /**
     * Receives up to max_parts messages available on socket
     * (but not less than min_parts).
     * Assume that length of part_names == min_parts
     */
    Incoming <RoutingPolicy>&
    receive_up_to(size_t min_parts, const char* part_names[],
      size_t max_parts) throw (MessageFormatError, ZmqErrorType);

    /**
     * Fetch all messages starting from tail message
     * until there will be no more parts on socket.
     * So the resulting data will be:
     * { parts_[N-1] | message, ... }
     * @param area - result stored here
     * @param delimiter - null terminated string, inserted between parts
     * @return number of messages in result (min 1)
     */
    int
    fetch_tail(std::vector<char>& area, const char* delimiter = 0)
    throw (ZmqErrorType);

    /**
     * Fetch all messages starting from tail message and drop them
     * until there will be no more parts on socket.
     * @return number of messages dropped (min 0)
     */
    int
    drop_tail() throw(ZmqErrorType);

    /**
     * After we have received message parts, we can extract
     * message parts content into variables, one by one.
     * Message content is copied for types that actually hold data,
     * not just wrap it.
     */
    template <typename T>
    Incoming<RoutingPolicy>&
    operator>> (T& t) throw(NoSuchPartError);

    /**
     * Extract following message part's content
     * by copying it into given message
     */
    Incoming<RoutingPolicy>&
    operator>> (zmq::message_t& msg) throw(NoSuchPartError);

    /**
     * Handle a manipulator
     */
    Incoming<RoutingPolicy>&
    operator>> (Incoming<RoutingPolicy>& (*f)(Incoming<RoutingPolicy>&))
    {
      return f(*this);
    }

    /**
     * Set stream mode to Binary:
     * all subsequent non-string data is extracted from ZMQ message as binary data
     * See \ref zm_modes "modes"
     */
    void
    set_binary()
    {
      binary_mode_ = true;
    }

    /**
     * Set stream mode to Text (default):
     * all subsequent non-string data is converted to string
     * (by writing to stream) after being extracted from ZMQ message.
     * See \ref zm_modes "modes"
     */
    void
    set_text()
    {
      binary_mode_ = false;
    }
  };

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

  /**
   * @brief Base class for Outgoing message,
   * does not depend on Routing policy.
   *
   * After Outgoing message is created, it's functionality
   * (append and send messages, caching, etc.) does not depend
   * on RoutingPolicy template parameter,
   * so it may be referenced as Sink class.
   */
  class Sink : private Private::NonCopyable
  {
  public:
    /**
     * @brief Output iterator to facilitate inserting of message parts
     * into outgoing message.
     */
    template <typename T>
    class iterator
    {
    public:
      typedef T value_type;
      typedef T& reference;
      typedef T* pointer;
      typedef std::ptrdiff_t difference_type;
      typedef std::output_iterator_tag iterator_category;

    private:
      class AssignProxy
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

    std::auto_ptr<Multipart> outgoing_queue_;

    /**
     * Keep message part until next part or flush
     *  to determine if the part is the latest in the message
     */
    MsgPtr cached_;

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
      outgoing_queue_(0), cached_(0), state_(NOTSENT),
      pending_routing_parts_(0)
    {}

    inline
    void
    add_pending_routing_part()
    {
      ++pending_routing_parts_;
    }

    inline
    unsigned
    options() const
    {
      return options_;
    }

    void
    send_one(
      zmq::message_t* msg, bool use_copy = false)
      throw(ZmqErrorType);

  private:
    void
    send_owned(zmq::message_t* owned) throw(ZmqErrorType);

    void
    do_send_one(
      zmq::message_t* msg, bool last) throw(ZmqErrorType);

    bool
    try_send_first_cached(
      bool last) throw(ZmqErrorType);

    void
    add_to_queue(zmq::message_t* msg);

  public:
    virtual
    ~Sink();

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
      ReceiveObserver* receive_observer = 0)
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
    operator<< (zmq::message_t& msg) throw (ZmqErrorType);

    /**
     * Either, we take ownership on message.
     * Not checking if this message is part of incoming_.
     * If ptr contains 0, null message is sent
     */
    inline Sink&
    operator<< (MsgPtr msg) throw (ZmqErrorType)
    {
      send_owned(msg.get() ? msg.release() : new zmq::message_t(0));
      return *this;
    }

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
  class Outgoing : public Sink
  {
  private:

    void
    send_routing(
      MsgPtrVec* routing) throw(ZmqErrorType);

  public:

    using Sink::iterator;

    /**
     * Routing policy used for sending multipart message
     */
    typedef RoutingPolicy RoutingPolicyType;

    Outgoing(zmq::socket_t& dst, unsigned options) :
      Sink(dst, options)
    {
      send_routing(0);
    }

    explicit
    Outgoing(OutOptions out_opts) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer)
    {
      send_routing(0);
    }

    /**
     * Outgoing message is a response to the given Incoming message,
     * so we resend Incoming's routing first.
     */
    template <typename InRoutingPolicy>
    Outgoing(zmq::socket_t& dst,
      Incoming<InRoutingPolicy>& incoming,
      unsigned options) throw(ZmqErrorType) :
      Sink(dst, options, 0, &incoming)
    {
      send_routing(incoming.get_routing());
    }

    /**
     * Outgoing message is a response to the given Incoming message,
     * so we resend Incoming's routing first.
     */
    template <typename InRoutingPolicy>
    Outgoing(OutOptions out_opts,
      Incoming<InRoutingPolicy>& incoming) throw(ZmqErrorType) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer, &incoming)
    {
      send_routing(incoming.get_routing());
    }

    /**
     * Outgoing message is NOT a response to the given Incoming message,
     * so we send normal routing.
     */
    Outgoing(zmq::socket_t& dst, Multipart& incoming,
      unsigned options) throw(ZmqErrorType) :
      Sink(dst, options, 0, &incoming)
    {
      send_routing(0);
    }

    /**
     * Outgoing message is NOT a response to the given Incoming message,
     * so we send normal routing.
     */
    Outgoing(OutOptions out_opts, Multipart& incoming) throw(ZmqErrorType) :
      Sink(out_opts.sock, out_opts.options, out_opts.send_observer, &incoming)
    {
      send_routing(0);
    }
  };
}

#endif /* ZMQMESSAGE_HPP_ */

#include "zmqmessage/ZmqMessageTemplateImpl.hpp"

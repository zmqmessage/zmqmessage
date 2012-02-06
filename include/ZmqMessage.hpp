/**
 * @file ZmqMessage.hpp
 * @author askryabin
 *
 */

#include <cstddef>
#include <vector>
#include <tr1/array>
#include <string>
#include <memory>

#include <zmq.hpp>

#include <zmqmessage/Config.hpp>
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

    inline
    void
    log_routing_received() const
    {
      ZMQMESSAGE_LOG_STREAM << "Receiving multipart, route received: "
        << routing_.size() << " parts";
    }

    ~XRouting();
  };

  class Multipart;

  template <class RoutingPolicy>
  class Outgoing;

  /**
   * Send given message to destination socket
   */
  void
  send(zmq::socket_t& sock, Multipart& multipart, bool nonblock)
    throw(ZmqErrorType);

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

    template <class RoutingPolicy>
    friend class Outgoing;

    friend void
    send(zmq::socket_t& sock, Multipart& multipart, bool nonblock)
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
    inline iterator<ZMQMESSAGE_STRING_CLASS>
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

  };

  /**
   * @brief Null (empty) message marker
   *
   * When passed to @c Outgoing (operator <<)
   * creates null (empty) message part
   */
  struct NullMessage {};

  /**
   * @brief Manipulator to flush outgoing message.
   *
   * Manipulator to finally flush (send/enqueue) the outgoing message.
   */
  struct Flush {};

  /**
   * @brief Manipulator to switch to binary mode
   *
   * Manipulator to turn off conversion to/from text sending/receiving
   * See \ref zm_modes "modes"
   */
  struct Binary {};

  /**
   * @brief Manipulator to switch to text mode (default)
   *
   * Manipulator to turn on conversion to/from text sending/receiving.
   * See \ref zm_modes "modes"
   */
  struct Text {};

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

    void
    append_message_data(
      zmq::message_t& message, std::vector<char>& area) const;

    /**
     * Fetches one message from src_, appends message to parts_.
     * @return if we have more messages on socket
     */
    bool
    receive_one() throw(ZmqErrorType);

    template <class OutRoutingPolicy>
    friend class Outgoing;

    inline
    MsgPtrVec*
    get_routing()
    {
      return RoutingPolicy::get_routing();
    }

  public:
    typedef RoutingPolicy RoutingPolicyType;

    explicit Incoming(zmq::socket_t& sock)
        : src_(sock), is_terminal_(false),
          cur_extract_idx_(0), binary_mode_(false)
    {
    }

    /**
     * @return zmq socket to receive message parts from
     */
    zmq::socket_t&
    src()
    {
      return src_;
    }

    /**
     * @return true if we have detected, that no more message parts are
     * accessible on socket (all parts are received).
     */
    inline bool
    is_terminal() const { return is_terminal_; }

    /**
     * Receive definite number of message parts.
     * @param parts number of parts to receive
     * @param part_names array of parts names
     * for error reporting and debug purposes
     * @param part_names_length length of part_names array
     * @param check_terminal if true, message may contain not more then
     * @c parts number of parts.
     */
    void
    receive(
      size_t parts, const char* part_names[],
      size_t part_names_length, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     */
    template <size_t N>
    inline void
    receive(
      std::tr1::array<const char*, N> part_names, bool check_terminal)
      throw (MessageFormatError)
    {
      receive(N, part_names.data(), check_terminal);
    }

    /**
     * @overload
     * Part names are omitted
     */
    inline void
    receive(size_t parts, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType)
    {
      receive(parts, 0, 0, check_terminal);
    }

    /**
     * @overload
     * assume size of part_names == parts
     */
    inline void
    receive(
      size_t parts, const char* part_names[], bool check_terminal)
      throw (MessageFormatError, ZmqErrorType)
    {
      receive(parts, part_names, parts, check_terminal);
    }

    /**
     * Receives ALL messages available on socket.
     * (min @c min_parts, max unbounded).
     * Multipart message will be considered terminal.
     */
    void
    receive_all(
      const size_t min_parts,
      const char* part_names[], size_t part_names_length)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     * assume size of part_names == min_parts
     */
    inline void
    receive_all(const size_t min_parts, const char* part_names[])
      throw (MessageFormatError, ZmqErrorType)
    {
      receive_all(min_parts, part_names, min_parts);
    }

    /**
     * @overload
     * No minimum parts limit (may be 0)
     */
    inline void
    receive_all()
      throw (MessageFormatError, ZmqErrorType)
    {
      receive_all(0, 0, 0);
    }

    /**
     * @overload
     * Part names are unknown.
     */
    inline void
    receive_all(const size_t min_parts)
      throw (MessageFormatError, ZmqErrorType)
    {
      receive_all(min_parts, 0, 0);
    }

    /**
     * Receives up to max_parts messages available on socket
     * (but not less than min_parts).
     * Assume that length of part_names == min_parts
     */
    void
    receive_up_to(size_t min_parts, const char* part_names[],
      size_t max_parts) throw (MessageFormatError, ZmqErrorType);

    /**
     * Fetch all messages starting from tail message
     * until there will be no more parts on socket.
     * We assume that we have used THIS socket on receive() call.
     * So the resulting data will be:
     * { parts_[N-1] | message, ... }
     * @param area - result stored here
     * @param delimiter - null terminated string, inserted between parts
     * @return number of messages in result (min 1)
     */
    int fetch_tail(std::vector<char>& area, const char* delimiter = 0)
      throw (ZmqErrorType);

    /**
     * Fetch all messages starting from tail message and drop them
     * until there will be no more parts on socket.
     * We assume that we have used THIS socket on receive() call.
     * @return number of messages in result (min 1)
     */
    int drop_tail() throw(ZmqErrorType);

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
     * Set stream mode to Binary:
     * all subsequent non-string data is extracted from ZMQ message as binary data
     * See \ref zm_modes "modes"
     */
    Incoming<RoutingPolicy>&
    operator>> (Binary)
    {
      binary_mode_ = true;
      return *this;
    }

    /**
     * Set stream mode to Text (default):
     * all subsequent non-string data is converted to string
     * (by writing to stream) after being extracted from ZMQ message.
     * See \ref zm_modes "modes"
     */
    Incoming<RoutingPolicy>&
    operator>> (Text)
    {
      binary_mode_ = false;
      return *this;
    }
  };

  /**
   * @brief Options for outgoing message
   *
   * This struct holds both reference to socket and options.
   * Needed for convenient creation of @c Outgoing class.
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

    inline OutOptions(zmq::socket_t& sock_p, unsigned options_p) :
      sock(sock_p), options(options_p)
    {}
  };

  /**
   * @brief Outgoing message.
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
  class Outgoing : private Private::NonCopyable
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
        AssignProxy(Outgoing<RoutingPolicy>& outgoing)
            : outgoing_(outgoing)
        {
        }

        void
        operator=(const T& output_object)
        {
          outgoing_ << output_object;
        }

      private:
        Outgoing<RoutingPolicy>& outgoing_;
      };

    public:
      explicit
      iterator(Outgoing<RoutingPolicy>& outgoing)
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
      Outgoing<RoutingPolicy>& outgoing_;

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

    /**
     * If not null, the routing will be taken from it.
     * Either, when sending/inserting zmq messages to Outgoing,
     * the ownership will be disowned from incoming_.
     */
    Multipart* incoming_;

    std::auto_ptr<Multipart> outgoing_queue_;

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

    void
    send_routing(
      MsgPtrVec* routing) throw(ZmqErrorType);

    void
    send_one(
      zmq::message_t* msg, bool use_copy = false)
      throw(ZmqErrorType);

    void
    send_owned(
      zmq::message_t* owned) throw(ZmqErrorType);

    void
    do_send_one(
      zmq::message_t* msg, bool last) throw(ZmqErrorType);

    bool
    try_send_first_cached(
      bool last) throw(ZmqErrorType);

    void
    add_to_queue(zmq::message_t* msg);

  public:
    /**
     * Routing policy used for sending multipart message
     */
    typedef RoutingPolicy RoutingPolicyType;

    Outgoing(zmq::socket_t& dst, unsigned options) :
      dst_(dst), options_(options), incoming_(0),
      outgoing_queue_(0), cached_(0), state_(NOTSENT)
    {
      send_routing(0);
    }

    explicit Outgoing(OutOptions out_opts) :
      dst_(out_opts.sock), options_(out_opts.options), incoming_(0),
      outgoing_queue_(0), cached_(0), state_(NOTSENT)
    {
      send_routing(0);
    }

    template <typename InRoutingPolicy>
    Outgoing(zmq::socket_t& dst,
      Incoming<InRoutingPolicy>& incoming,
      unsigned options) throw(ZmqErrorType):
      dst_(dst), options_(options), incoming_(&incoming),
      outgoing_queue_(0), cached_(0), state_(NOTSENT)
    {
      send_routing(incoming.get_routing());
    }

    template <typename InRoutingPolicy>
    Outgoing(OutOptions out_opts,
      Incoming<InRoutingPolicy>& incoming) throw(ZmqErrorType) :
      dst_(out_opts.sock), options_(out_opts.options), incoming_(&incoming),
      outgoing_queue_(0), cached_(0), state_(NOTSENT)
    {
      send_routing(incoming.get_routing());
    }

    ~Outgoing();

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

    /**
     * Send all messages contained in @c incoming_ starting from idx_from
     */
    void
    send_incoming_messages(size_t idx_from = 0) throw(ZmqErrorType);

    /**
     * Receive and send/enqueue pending messages from relay_src socket
     */
    void
    relay_from(zmq::socket_t& relay_src) throw(ZmqErrorType);

    /**
     * Receive and send/enqueue pending messages from relay_src socket,
     * counting sizes of received messages
     * @tparam OccupationAccumulator unary functor accepting size_t
     */
    template <class OccupationAccumulator>
    void
    relay_from(
      zmq::socket_t& relay_src, OccupationAccumulator acc)
      throw (ZmqErrorType);

    /**
     * Lowest priority in overload. Uses @c ZmqMessage::init_msg functions
     * to compose message part and send.
     */
    template <typename T>
    Outgoing<RoutingPolicy>&
    operator<< (const T& t);

    /**
     * Note, we take ownership on message
     * (unless message is part of incoming_ and COPY_INCOMING option is set).
     * We invalidate incoming_'s ownership on this message
     * (if COPY_INCOMING option is NOT set).
     * Do not pass stack-allocated messages
     * if this object is planned to be detached and used beyond current block.
     */
    Outgoing<RoutingPolicy>&
    operator<< (zmq::message_t& msg);

    /**
     * Either, we take ownership on message.
     * Not checking if this message is part of incoming_.
     * If ptr contains 0, null message is sent
     */
    inline Outgoing<RoutingPolicy>&
    operator<< (MsgPtr msg)
    {
      send_owned(msg.get() ? msg.release() : new zmq::message_t(0));
      return *this;
    }

    /**
     * Insert null (empty) message part
     */
    inline Outgoing<RoutingPolicy>&
    operator<< (NullMessage tag)
    {
      send_owned(new zmq::message_t(0));
      return *this;
    }

    /**
     * Insert raw message (see @c RawMessage)
     */
    Outgoing<RoutingPolicy>&
    operator<< (const RawMessage& m);

    /**
     * Insert Flush manipulator to flush (finally send/enqueue) the message.
     */
    inline Outgoing<RoutingPolicy>&
    operator<< (Flush)
    {
      flush();
      return *this;
    }

    /**
     * Switch stream to binary mode. See \ref zm_modes "modes"
     */
    inline Outgoing<RoutingPolicy>&
    operator<< (Binary)
    {
      options_ |= OutOptions::BINARY_MODE;
      return *this;
    }

    /**
     * Switch stream to text mode. See \ref zm_modes "modes"
     */
    inline Outgoing<RoutingPolicy>&
    operator<< (Text)
    {
      options_ &= ~OutOptions::BINARY_MODE;
      return *this;
    }

  };

}

#endif /* ZMQMESSAGE_HPP_ */

#include "zmqmessage/ZmqMessageImpl.hpp"


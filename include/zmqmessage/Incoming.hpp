/**
 * @file Incoming.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_INCOMING_HPP_
#define ZMQMESSAGE_INCOMING_HPP_

#include <vector>
#include <tr1/array>

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/MultipartContainer.hpp>
#include <zmqmessage/Routing.hpp>
#include <zmqmessage/Manip.hpp> //Skip is friend

namespace ZmqMessage
{
  /**
   * @brief Incoming multipart ZMQ message.
   *
   * @tparam RoutingPolicy either SimpleRouting or XRouting -
   * rules for receiving routing info.
   */
  template <class RoutingPolicy, class PartsStorage = DynamicPartsStorage<> >
  class Incoming :
    private RoutingPolicy,
    public Private::MultipartContainer<PartsStorage>
  {
  public:
    typedef Incoming<RoutingPolicy, PartsStorage> SelfType;

    using Multipart::size;

  private:
    typedef Private::MultipartContainer<PartsStorage> ContainerType;

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
    receive_one() throw(ZmqErrorType, MessageFormatError);

    bool
    do_receive_msg(Part& part) throw(ZmqErrorType);

    template <class OutRoutingPolicy>
    friend class Outgoing;

    using RoutingPolicy::get_routing;
    using RoutingPolicy::get_routing_num;

    friend
    SelfType&
    Skip<RoutingPolicy, PartsStorage>(SelfType&);

  public:

    typedef typename PartsStorage::StorageArg StorageArg;

    /**
     * @param sock source socket to receive from
     * @param arg Storage-dependent argument, such as initial storage capacity
     */
    Incoming(zmq::socket_t& sock,
      StorageArg arg = PartsStorage::default_storage_arg) :
      ContainerType(arg),
      src_(sock), is_terminal_(false),
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
     * @return receive observer previously set with set_receive_observer
     * or 0 if it's not set.
     */
    inline
    ReceiveObserver*
    receive_observer()
    {
      return receive_observer_;
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
     * Number of variables extracted (with operator >>) from this multipart.
     */
    size_t
    extracted() const
    {
      return cur_extract_idx_;
    }

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
    SelfType&
    receive(
      size_t parts, const char* part_names[],
      size_t part_names_length, bool check_terminal)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     */
    template <size_t N>
    inline
    SelfType&
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
    SelfType&
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
    SelfType&
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
    SelfType&
    receive_all(
      const size_t min_parts,
      const char* part_names[], size_t part_names_length)
      throw (MessageFormatError, ZmqErrorType);

    /**
     * @overload
     * assume size of part_names == min_parts
     */
    inline
    SelfType&
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
    SelfType&
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
    SelfType&
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
    SelfType&
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
    SelfType&
    operator>> (T& t) throw(NoSuchPartError);

    /**
     * Extract following message part's content
     * by copying it into given message
     */
    SelfType&
    operator>> (zmq::message_t& msg) throw(NoSuchPartError);

    /**
     * Handle a manipulator
     */
    SelfType&
    operator>> (SelfType& (*f)(SelfType&))
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

}
#endif /* ZMQMESSAGE_INCOMING_HPP_ */

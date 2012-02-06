#ifndef ZMQMESSAGE_ZMQ_TOOLS_INCLUDED_
#define ZMQMESSAGE_ZMQ_TOOLS_INCLUDED_

#include <cstdlib>
#include <ctime>

#include <string>
#include <sstream>

#include "zmqmessage/Config.hpp"
#include "zmqmessage/MetaTypes.hpp"
#include "zmqmessage/RawMessage.hpp"

namespace ZmqMessage
{
  /**
   * Get message contents
   * For string messages (containing characters, possibly not null-terminated)
   * @param message zmq message
   * @tparam T class supporting string operations (@see Private::IsStr)
   */
  template <typename T>
  inline
  T
  get(zmq::message_t& message, T&,
    typename Private::EnableIf<Private::IsStr<T>::value>::type* = 0)
  {
    return T(static_cast<char*>(message.data()), message.size());
  }

  /**
   * Get message contents
   * For non-string types.
   * Use for text messages with content writable to stream and readable as type T.
   * (ex. if T is int: "678" -> 678)
   * @param message zmq message
   * @return the converted content
   */
  template <typename T>
  inline
  T
  get(zmq::message_t& message, T&,
      typename Private::DisableIf<Private::IsStr<T>::value>::type* = 0,
      typename Private::DisableIf<Private::IsRaw<T>::value>::type* = 0)
  {
    std::stringstream ss;
    ss.write(static_cast<char*>(message.data()), message.size());
    T t;
    ss >> t;
    return t;
  }

  template <typename T>
  inline
  T&
  get(zmq::message_t& message, T& t,
      typename Private::DisableIf<Private::IsStr<T>::value>::type* = 0,
      typename Private::EnableIf<Private::IsRaw<T>::value>::type* = 0)
  {
    get_bin(message, t);
    return t;
  }

  /**
   * Put binary message contents into existing variable.
   * For binary messages containing elementary types or properly aligned PODs
   * @param message zmq message
   * @param t will contain the COPY of message contents
   */
  template <typename T>
  inline
  void
  get_bin(zmq::message_t& message, T& t)
  {
    t = *(reinterpret_cast<T*>(message.data()));
  }

  /**
   * Return variable with binary message contents.
   * For binary messages containing elementary types or properly aligned PODs
   * @param message zmq message
   */
  template <typename T>
  inline T
  get_bin(zmq::message_t& message)
  {
    return *(reinterpret_cast<T*>(message.data()));
  }

  /**
   * Return memory region as RawMessage. No copying takes place.
   */
  inline RawMessage
  get_raw(zmq::message_t& message)
  {
    return RawMessage(message.data(), message.size());
  }

  /**
   * Extract string class from text message (possibly not zero-terminated)
   */
  template <typename T>
  inline T
  get_string(zmq::message_t& message)
  {
    return T(static_cast<char*>(message.data()), message.size());
  }

  /**
   * @overload
   */
  inline ZMQMESSAGE_STRING_CLASS
  get_string(zmq::message_t& message)
  {
    return get_string<ZMQMESSAGE_STRING_CLASS>(message);
  }

  /**
   * @overload
   * Extract up to @c limit characters from text message
   * (possibly not zero-terminated)
   */
  template <typename T>
  inline T
  get_string(zmq::message_t& message, size_t limit)
  {
    return T(
      static_cast<char*>(message.data()), std::min(message.size(), limit)
    );
  }

  /**
   * @overload
   */
  inline ZMQMESSAGE_STRING_CLASS
  get_string(zmq::message_t& message, size_t limit)
  {
    return get_string<ZMQMESSAGE_STRING_CLASS>(message, limit);
  }

  /**
   * Get timestamp from TEXT message, converting ASCII string to unsigned long
   */
  ZMQMESSAGE_HEADERONLY_INLINE
  time_t
  get_time(zmq::message_t& message);

  /**
   * Compare message contents to specified memory region.
   * @return like @c memcmp
   */
  inline int
  msgcmp(zmq::message_t& message, const char* str, size_t len)
  {
    int ret = memcmp(message.data(), str, std::min(message.size(), len));
    return ret ? ret : message.size() - len;
  }

  /**
   * Compare message contents to specified zero-terminated C string.
   * @return like @c memcmp
   */
  inline int
  msgcmp(zmq::message_t& message, const char* str)
  {
    return msgcmp(message, str, strlen(str));
  }

  /**
   * Compare message contents to specified string.
   * @return like @c memcmp
   */
  template <typename T>
  inline int
  msgcmp(zmq::message_t& message, const T& str,
    typename Private::EnableIf<Private::IsStr<T>::value>::type* = 0)
  {
    return msgcmp(message, str.data(), str.length());
  }

  /**
   * Initialize zmq message with copy of given data.
   * (created duplicate of buffer will be owned by zmq message
   * and will be destroyed by it)
   */
  ZMQMESSAGE_HEADERONLY_INLINE
  void
  init_msg(const void* t, size_t sz, zmq::message_t& msg);

  /**
   * Initialize zmq message by string object.
   * No null terminator (if any) appended to message.
   */
  template <class T>
  inline void
  init_msg(const T& t, zmq::message_t& msg,
    typename Private::EnableIf<Private::IsStr<T>::value>::type* = 0)
  {
    init_msg(t.data(), t.length(), msg);
  }

  /*
   * Initialize zmq message by arbitrary (non-string) type that can be
   * written to output stream.
   */
  template <class T>
  inline void
  init_msg(const T& t, zmq::message_t& msg,
           typename Private::DisableIf<Private::IsStr<T>::value>::type* = 0,
           typename Private::DisableIf<Private::IsRaw<T>::value>::type* = 0)
  {
    std::ostringstream os;
    os << t;
    const std::string& s = os.str();
    init_msg(s, msg);
  }

  /*
   * Initialize zmq message by arbitrary (non-string) type to send
   * in binary form
   */
  template <class T>
  inline void
  init_msg(const T& t, zmq::message_t& msg,
           typename Private::DisableIf<Private::IsStr<T>::value>::type* = 0,
           typename Private::EnableIf<Private::IsRaw<T>::value>::type* = 0)
  {
    init_msg(&t, sizeof(T), msg);
  }

  /**
   * Initialize zmq message with zero-terminated string
   */
  inline void
  init_msg(const char* t, zmq::message_t& msg)
  {
    init_msg(t, strlen(t), msg);
  }


  /**
   * throw exception: either unwrapped zmq::error_t or wrapped exception,
   * depending on configuration
   */
  inline void
  throw_zmq_exception(const zmq::error_t& e)
    throw(ZmqErrorType)
  {
#ifdef ZMQMESSAGE_WRAP_ZMQ_ERROR
    throw ZmqErrorType(e.what());
#else
    throw e;
#endif
  }

  /**
   * Receive message part from socket
   */
  inline void
  recv_msg(zmq::socket_t& sock, zmq::message_t& msg,
    int flags = 0) throw(ZmqErrorType)
  {
    try
    {
      if (!sock.recv(&msg, flags))
      {
        throw zmq::error_t();
      }
    }
    catch (const zmq::error_t& e)
    {
      throw_zmq_exception(e);
    }
  }

  /**
   * Send message part to socket with specified flags
   */
  inline void
  send_msg(zmq::socket_t& sock, zmq::message_t& msg, int flags)
    throw(ZmqErrorType)
  {
    try
    {
      if (!sock.send(msg, flags))
      {
        throw zmq::error_t();
      }
    }
    catch (const zmq::error_t& e)
    {
      throw_zmq_exception(e);
    }
  }

  /**
   * Copy contents of one message part to another
   */
  inline void
  copy_msg(zmq::message_t& target, zmq::message_t& source)
    throw(ZmqErrorType)
  {
    try
    {
      target.copy(&source);
    }
    catch (const zmq::error_t& e)
    {
      throw_zmq_exception(e);
    }
  }

  /**
   * Does specified socket has more messages to receive
   */
  ZMQMESSAGE_HEADERONLY_INLINE
  bool
  has_more(zmq::socket_t& sock);

  /**
   * Relays all pending messages (until no more)
   * from src to dst
   * @param src source socket
   * @param dst destination socket
   * @param check_first_part true to check if we have more parts
   * even for 1st part
   * @return number of relayed parts
   */
  ZMQMESSAGE_HEADERONLY_INLINE
  int
  relay_raw(zmq::socket_t& src, zmq::socket_t& dst,
    bool check_first_part = true);
}


#endif //ZMQMESSAGE_ZMQ_TOOLS_INCLUDED_


#ifdef ZMQMESSAGE_HEADERONLY
# include "zmqmessage/ZmqToolsFullImpl.hpp"
#endif

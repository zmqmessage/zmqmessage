/**
 * @file Config.hpp
 * @author askryabin
 * @brief Basic compile time configurations for ZmqMessage library
 */

#include "zmq.hpp"

#ifndef ZMQMESSAGE_CONFIG_HPP_
#define ZMQMESSAGE_CONFIG_HPP_

//You can (and encouraged to) define the following constants
//to adapt ZmqMessage library to your application.

/**
 * @def ZMQMESSAGE_STRING_CLASS
 * Class that is default string representation.
 * Must be refinement of a string concept.
 * String concept requires a class to be both
 * constructible on const char* and size_t
 * and have data() and length() accessor methods:
 * @code
 * class MyString
 * {
 *  MyString(const char*, size_t);
 *  const char* data() const;
 *  size_t length() const;
 * };
 * @endcode
 * It's possible (and desirable) for it not to copy and own the memory,
 * but just wrap the external memory region.
 * By default, std::string is used.
 */
#ifndef ZMQMESSAGE_STRING_CLASS
#include <string>
#define ZMQMESSAGE_STRING_CLASS std::string
#endif

/**
 * @def ZMQMESSAGE_LOG_STREAM_NONE
 * Use this constant to disable logging completely.
 * @code
 * #define ZMQMESSAGE_LOG_STREAM ZMQMESSAGE_LOG_STREAM_NONE
 * @endcode
 * jkl
 */
#define ZMQMESSAGE_LOG_STREAM_NONE if(1); else std::cerr

/**
 * @def ZMQMESSAGE_LOG_STREAM
 * Variable or macro to stream log messages to (with operator <<).
 * This enables you to adapt ZmqMessage to you logging system.
 * By default, std::cerr is used (when NDEBUG macro is set),
 * otherwise ZmqMessage is silent.
 * If you want to disable logging even for debug builds,
 * use \ref ZMQMESSAGE_LOG_STREAM_NONE
 * @code
 * #define ZMQMESSAGE_LOG_STREAM ZMQMESSAGE_LOG_STREAM_NONE
 * @endcode
 */
#ifndef ZMQMESSAGE_LOG_STREAM
//just to generate correct docs
#define ZMQMESSAGE_LOG_STREAM aa
#include <iostream>
# ifdef NDEBUG
#  define ZMQMESSAGE_LOG_STREAM ZMQMESSAGE_LOG_STREAM_NONE
# else
#  define ZMQMESSAGE_LOG_STREAM std::cerr
# endif
#endif

/**
 * @def ZMQMESSAGE_LOG_TERM
 * Streamable literal that is appended (<<) to every log record.
 * If your logging system doesn-t require terminal (such as new line) you can
 * @code
 * #define ZMQMESSAGE_LOG_TERM ""
 * @endcode
 */
#ifndef ZMQMESSAGE_LOG_TERM
#define ZMQMESSAGE_LOG_TERM std::endl
#endif

/**
 * @def ZMQMESSAGE_EXCEPTION_MACRO
 * Macro to generate exception class definition by exception name.
 * It enables you to adapt ZmqMessage to you application exception policy
 * (and hierarchy).
 * By default, we generate simple derivative from std::logic_error
 */
#ifndef ZMQMESSAGE_EXCEPTION_MACRO
#include <stdexcept>
#define ZMQMESSAGE_EXCEPTION_MACRO(name) \
  class name : public std::logic_error \
  { \
  public: \
    explicit name (const std::string& arg) : \
      std::logic_error(arg) {} \
  }
#endif

/**
 * @def ZMQMESSAGE_WRAP_ZMQ_ERROR
 * if defined, zmq::error_t will be wreapped
 * with exception generated according to \ref ZMQMESSAGE_EXCEPTION_MACRO
 * and named @c ZmqException.
 * Otherwise, it's thrown upwards as is.
 */
namespace ZmqMessage
{
#ifndef ZMQMESSAGE_WRAP_ZMQ_ERROR

//just to generate correct docs
# define ZMQMESSAGE_WRAP_ZMQ_ERROR 1
# undef ZMQMESSAGE_WRAP_ZMQ_ERROR

  typedef zmq::error_t ZmqErrorType;
  inline void
  throw_zmq_error(const char* err)
  {
    throw zmq::error_t();
  }
#else
  ZMQMESSAGE_EXCEPTION_MACRO(ZmqException)
  ;
  typedef ZmqException ZmqErrorType;
  inline void
  throw_zmq_error(const char* err)
  {
    throw ZmqException(err);
  }
#endif

}

#endif /* ZMQMESSAGE_CONFIG_HPP_ */

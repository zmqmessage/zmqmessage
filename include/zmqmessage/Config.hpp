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
 * @def ZMQMESSAGE_HEADERONLY
 * First of all, you need to choose build type: header-only
 * (which gives more flexibility in settings)
 * or linking against shared library (which gives less flexibility
 * in configuration, but reduces compile time, size of generated executable
 * and may appear to be more efficient).
 * By defining this constant you perform header-only build and do not need
 * to link against shared library:
 * \code
 * #define ZMQMESSAGE_HEADERONLY
 * \endcode
 */
#ifndef ZMQMESSAGE_HEADERONLY
//just to generate correct docs
# define ZMQMESSAGE_HEADERONLY 1
# undef ZMQMESSAGE_HEADERONLY
# define ZMQMESSAGE_HEADERONLY_INLINE
#else
# define ZMQMESSAGE_HEADERONLY_INLINE inline
#endif

/**
 * @def ZMQMESSAGE_STRING_CLASS
 * Class that is default string representation.
 * Must satisfy the requirements of the string concept.
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
 * but just wrap the external memory region. For example, take a look at
 * StringFace class from examples.
 * By default, std::string is used.
 * You may define this setting both if you are linking against prebuilt
 * shared library or using library as header-only
 * (see \ref ZMQMESSAGE_HEADERONLY)
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
 * Note, that this constant is overridable either for
 * \ref ZMQMESSAGE_HEADERONLY "headeronly" builds
 * or during compiling shared library.
 * Defining this setting in your application headers during
 * non header-only build takes no effect.
 */
#ifndef ZMQMESSAGE_LOG_STREAM
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
 * Note, that this constant is overridable either for
 * \ref ZMQMESSAGE_HEADERONLY "headeronly" builds
 * or during compiling shared library.
 * Defining this setting in your application headers during
 * non header-only build takes no effect.
 */
#ifndef ZMQMESSAGE_LOG_TERM
#define ZMQMESSAGE_LOG_TERM std::endl
#endif

/**
 * @def ZMQMESSAGE_EXCEPTION_MACRO
 * Macro to generate exception class definition by exception name.
 * It enables you to adapt ZmqMessage to you application exception policy
 * (and hierarchy).
 * By default, we generate simple derivative from std::logic_error.
 * Note, that final semicolon (';') will be appended at end
 * where macro is used, so it's not needed in macro definition.
 * Note, that this constant is overridable either for
 * \ref ZMQMESSAGE_HEADERONLY "headeronly" builds
 * or during compiling shared library.
 * For non header-only builds:
 * This setting MUST be the same as used while building shared library.
 * So, if you rebuild library with specific (non-default) exception
 * macro definition, you MUST provide the SAME definitions before including
 * the library headers in your application.
 * Otherwise, the unexpected exception would be thrown, it may lead to crash.
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
 * if defined, @c zmq::error_t will be wrapped
 * with exception generated according to \ref ZMQMESSAGE_EXCEPTION_MACRO
 * and named @c ZmqException.
 * Otherwise, it's thrown upwards as is.
 * For non header-only builds must be the same in shared library
 * and client code, just like \ref ZMQMESSAGE_EXCEPTION_MACRO
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

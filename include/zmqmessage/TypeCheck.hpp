/**
 * @file TypeCheck.hpp
 * @author askryabin
 * Compile-time type checks
 */

#ifndef ZMQMESSAGE_TYPECHECK_HPP_
#define ZMQMESSAGE_TYPECHECK_HPP_

namespace ZmqMessage
{
  namespace Private
  {
    /**
     * Checking that all generated classes are the same types
     * in shared library and in included headers.
     */
    template <
      typename MessageFormatErrorT,
      typename NoSuchPartErrorT,
      typename ZmqErrorTypeT>
    struct TypeCheck
    {
      //defined in shared library for correct types
      static const int value;
    };

    namespace
    {
      const int type_check_ok = TypeCheck<
        MessageFormatError, NoSuchPartError, ZmqErrorType>::value;
    }
  }
}

#endif /* ZMQMESSAGE_TYPECHECK_HPP_ */

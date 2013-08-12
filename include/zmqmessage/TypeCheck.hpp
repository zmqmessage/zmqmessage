/**
 * @file TypeCheck.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Compile-time type checks
 */

#ifndef ZMQMESSAGE_TYPECHECK_HPP_
#define ZMQMESSAGE_TYPECHECK_HPP_

#include <zmqmessage/Config.hpp>

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
    struct ZMQMESSAGE_DLL_PUBLIC TypeCheck
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

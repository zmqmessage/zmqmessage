/**
 * @file exceptions.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_EXCEPTIONS_HPP_
#define ZMQMESSAGE_EXCEPTIONS_HPP_

#include <ZmqMessageFwd.hpp>
#include <zmqmessage/Config.hpp>

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
}

#endif /* ZMQMESSAGE_EXCEPTIONS_HPP_ */

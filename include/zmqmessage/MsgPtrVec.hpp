/**
 * @file MsgPtrVec.hpp
 * @author askryabin
 *
 */

#include <vector>
#include <zmq.hpp>

#ifndef ZMQMESSAGE_MSGPTRVEC_HPP_
#define ZMQMESSAGE_MSGPTRVEC_HPP_

namespace ZmqMessage
{
  /**
   * Vector of pointers to message parts.
   */
  typedef std::vector<zmq::message_t*> MsgPtrVec;
}

#endif /* ZMQMESSAGE_MSGPTRVEC_HPP_ */

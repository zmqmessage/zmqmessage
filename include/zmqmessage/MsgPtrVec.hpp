/**
 * @file MsgPtrVec.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <nifigase@gmail.com>, et al.
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

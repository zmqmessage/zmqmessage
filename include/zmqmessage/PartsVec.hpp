/**
 * @file MsgPtrVec.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 */

#include <vector>
#include <zmqmessage/Part.hpp>

#ifndef ZMQMESSAGE_PARTSVEC_HPP_
#define ZMQMESSAGE_PARTSVEC_HPP_

namespace ZmqMessage
{
  /**
   * Vector of message parts.
   */
  typedef std::vector<Part> PartsVec;
}

#endif /* ZMQMESSAGE_PARTSVEC_HPP_ */

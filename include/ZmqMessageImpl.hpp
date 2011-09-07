/**
 * @file ZmqMessageImpl.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * This file generates all definitions.
 * It should be included in ONE .cpp file in your application
 * if you are not linking against ZmqMessage shared library.
 * Otherwise do not include it.
 */

#ifndef ZMQMESSAGEIMPL_HPP_
#define ZMQMESSAGEIMPL_HPP_

#include "ZmqMessage.hpp"
#include "ZmqTools.hpp"

#include "zmqmessage/ZmqMessageFullImpl.hpp"
#include "zmqmessage/ZmqToolsFullImpl.hpp"

namespace ZmqMessage
{
  //explicit template instantiations - it brings the code
  template class Incoming<SimpleRouting>;
  template class Incoming<XRouting>;
  template class Outgoing<SimpleRouting>;
  template class Outgoing<XRouting>;

  namespace Private
  {
    template<> const int TypeCheck<
      MessageFormatError, NoSuchPartError, ZmqErrorType>::value = 1;
  }
}

#endif /* ZMQMESSAGEIMPL_HPP_ */

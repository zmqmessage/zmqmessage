/**
 * @file ZmqMessage.cpp
 * @author askryabin
 *
 */

#include "ZmqMessage.hpp"

#include "zmqmessage/ZmqMessageFullImpl.hpp"

namespace ZmqMessage
{
  //explicit template instantiations - it brings the code
  template class Incoming<SimpleRouting>;
  template class Incoming<XRouting>;
  template class Outgoing<SimpleRouting>;
  template class Outgoing<XRouting>;
}

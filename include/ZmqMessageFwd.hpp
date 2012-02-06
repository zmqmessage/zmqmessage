/**
 * @file ZmqMessageFwd.hpp
 * @author askryabin
 * Forward declarations of ZmqMessage classes.
 */

#ifndef ZMQMESSAGE_ZMQMESSAGEFWD_HPP_
#define ZMQMESSAGE_ZMQMESSAGEFWD_HPP_

namespace ZmqMessage
{

  class SimpleRouting;

  class XRouting;

  class Multipart;

  template <class RoutingPolicy>
  class Incoming;

  struct OutOptions;

  template <class RoutingPolicy>
  class Outgoing;

}

#endif /* ZMQMESSAGE_ZMQMESSAGEFWD_HPP_ */

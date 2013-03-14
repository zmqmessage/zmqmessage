/**
 * @file ZmqMessageFwd.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Forward declarations of ZmqMessage classes.
 */

#ifndef ZMQMESSAGE_ZMQMESSAGEFWD_HPP_
#define ZMQMESSAGE_ZMQMESSAGEFWD_HPP_

namespace ZmqMessage
{
  class Part;

  //parts storage policies

  template <size_t N>
  class StackPartsStorage;

  template <typename Allocator = std::allocator<Part> >
  class DynamicPartsStorage;

  //routing policies

  class SimpleRouting;

  class XRouting;

  class Multipart;

  class ReceiveObserver;

  template <class RoutingPolicy, class PartsStorage>
  class Incoming;

  class SendObserver;

  struct OutOptions;

  class Sink;

  template <class RoutingPolicy>
  class Outgoing;

}

#endif /* ZMQMESSAGE_ZMQMESSAGEFWD_HPP_ */

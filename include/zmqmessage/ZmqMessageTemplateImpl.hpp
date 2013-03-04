/**
 * @file ZmqMessageTemplateImpl.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 * Definition of template functions.
 * They are to be instantiated in client code.
 */

#include <functional>
#include <algorithm>

#include "zmqmessage/TypeCheck.hpp"

#ifndef ZMQMESSAGE_ZMQMESSAGETEMPLATEIMPL_HPP_
#define ZMQMESSAGE_ZMQMESSAGETEMPLATEIMPL_HPP_

/////////////////////////////////////////////////
//       definition of templates
/////////////////////////////////////////////////

namespace ZmqMessage
{
  template <typename T>
  void
  Multipart::iterator<T>::set_cur()
  {
    if (end())
    {
      return;
    }
    if (!(*messages_)[idx_].valid())
    {
      cur_ = T();
      return;
    }

    get((*messages_)[idx_].msg(), cur_, binary_mode_);
  }

  template <class RoutingPolicy>
  template <typename T>
  Incoming<RoutingPolicy>&
  Incoming<RoutingPolicy>::operator>> (T& t) throw(NoSuchPartError)
  {
    check_has_part(cur_extract_idx_);
    get(parts_[cur_extract_idx_++].msg(), t, binary_mode_);
    return *this;
  }

  template <typename T>
  Sink&
  Sink::operator<< (const T& t) throw (ZmqErrorType)
  {
    Part part;
    bool binary_mode = options_ & OutOptions::BINARY_MODE;
    init_msg(t, part.msg(), binary_mode);
    send_owned(part);
    return *this;
  }

  template <class OccupationAccumulator>
  void
  Sink::relay_from(
    zmq::socket_t& relay_src, OccupationAccumulator acc,
    ReceiveObserver* receive_observer)
    throw (ZmqErrorType)
  {
    for (bool more = has_more(relay_src); more; )
    {
      Part cur_part;
      recv_msg(relay_src, cur_part.msg());
      more = has_more(relay_src);
      if (receive_observer)
      {
        receive_observer->on_receive_part(cur_part.msg(), more);
      }
      const size_t sz = cur_part.msg().size();
      acc(sz);
      send_owned(cur_part);
    }
  }
}

#endif /* ZMQMESSAGE_ZMQMESSAGETEMPLATEIMPL_HPP_ */

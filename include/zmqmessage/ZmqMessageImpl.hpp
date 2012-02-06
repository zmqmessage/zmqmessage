/**
 * @file ZmqMessageImpl.hpp
 * @author askryabin
 *
 */

#include <functional>
#include <algorithm>

#ifndef ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_
#define ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_

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
    if (messages_[idx_] == 0)
    {
      cur_ = T();
      return;
    }
    if (binary_mode_)
    {
      get_bin(*(messages_[idx_]), cur_);
    }
    else
    {
      cur_ = get<T>(*(messages_[idx_]));
    }
  }

  template <class RoutingPolicy>
  template <typename T>
  Incoming<RoutingPolicy>&
  Incoming<RoutingPolicy>::operator>> (T& t) throw(NoSuchPartError)
  {
    check_has_part(cur_extract_idx_);
    if (binary_mode_)
    {
      get_bin(*(parts_[cur_extract_idx_++]), t);
    }
    else
    {
      t = get<T>(*(parts_[cur_extract_idx_++]));
    }
    return *this;
  }

  template <class RoutingPolicy>
  template <typename T>
  Outgoing<RoutingPolicy>&
  Outgoing<RoutingPolicy>::operator<< (const T& t)
  {
    if (options_ & OutOptions::BINARY_MODE)
    {
      return operator<<(RawMessage(&t, sizeof(T)));
    }
    else
    {
      MsgPtr msg(new zmq::message_t);
      init_msg(t, *msg);
      send_owned(msg.release());
    }
    return *this;
  }

  template <class RoutingPolicy>
  template <class OccupationAccumulator>
  void
  Outgoing<RoutingPolicy>::relay_from(
    zmq::socket_t& relay_src, OccupationAccumulator acc)
    throw (ZmqErrorType)
  {
    while (has_more(relay_src))
    {
      MsgPtr cur_part(new zmq::message_t);
      recv_msg(relay_src, *cur_part);
      size_t sz = cur_part->size();
      acc(sz);
      send_owned(cur_part.release());
    }
  }
}

#endif /* ZMQMESSAGE_ZMQMESSAGEIMPL_HPP_ */

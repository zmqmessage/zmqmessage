/**
 * @file ZmqMessageTemplateImpl.hpp
 * @author askryabin
 * Definition of template functions.
 * They are to be instantiated in client code.
 */

#include <functional>
#include <algorithm>

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
      cur_ = get<T>(*(messages_[idx_]), cur_);
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
      t = get<T>(*(parts_[cur_extract_idx_++]), t);
    }
    return *this;
  }

  template <class RoutingPolicy>
  template <typename T>
  Outgoing<RoutingPolicy>&
  Outgoing<RoutingPolicy>::operator<< (const T& t) throw (ZmqErrorType)
  {
    MsgPtr msg(new zmq::message_t);
    if (options_ & OutOptions::BINARY_MODE)
    {
      init_msg(&t, sizeof(T), *msg);
    }
    else
    {
      init_msg(t, *msg);
    }
    send_owned(msg.release());
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

#endif /* ZMQMESSAGE_ZMQMESSAGETEMPLATEIMPL_HPP_ */

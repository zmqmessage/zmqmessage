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
#include "zmqmessage/ScopedAlloc.hpp"

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
    if (!(*multipart_)[idx_].valid())
    {
      cur_ = T();
      return;
    }

    get((*multipart_)[idx_].msg(), cur_, binary_mode_);
  }

  template <typename Allocator>
  DynamicPartsStorage<Allocator>::DynamicPartsStorage(size_t capacity) :
    parts_(Allocator::allocate(capacity)),
    capacity_(capacity),
    size_(0)
  {}

  template <typename Allocator>
  DynamicPartsStorage<Allocator>::DynamicPartsStorage() :
    parts_(0),
    capacity_(0),
    size_(0)
  {}

  template <typename Allocator>
  DynamicPartsStorage<Allocator>::~DynamicPartsStorage()
  {
    if (parts_)
    {
      for (size_t i = 0; i < size_; ++i)
      {
        Allocator::destroy(&(parts_[i]));
      }
      Allocator::deallocate(parts_, capacity_);
    }
  }

  template <typename Allocator>
  Part*
  DynamicPartsStorage<Allocator>::next()
  {
    if (!parts_)
    {
      return 0;
    }

    if (size_ == capacity_)
    {
      const size_t new_capacity = capacity_ << 1;
      Private::ScopedAlloc<Allocator> alloc(new_capacity);

      for (size_t i = 0; i < size_; ++i)
      {
        Allocator::construct(&(alloc.mem()[i]), parts_[i]);
        Allocator::destroy(&(parts_[i]));
      }

      Allocator::deallocate(parts_, capacity_);
      parts_ = alloc.release();
      capacity_ = new_capacity;
    }

    //XXX cannot use Allocator::construct to create object without prototype
    Part* const ptr = &(parts_[size_]);
    ::new(ptr) Part();
    ++size_;
    return ptr;
  }

  template <typename Allocator>
  Multipart*
  DynamicPartsStorage<Allocator>::detach()
  {
    typedef Private::MultipartContainer<DynamicPartsStorage<Allocator> > Cont;
    std::auto_ptr<Cont> ptr(new Cont());
    ptr->parts_ = parts_;
    ptr->capacity_ = capacity_;
    ptr->size_ = size_;
    parts_ = 0;
    capacity_ = 0;
    size_ = 0;
    return ptr.release();
  }

  template <class RoutingPolicy, class PartsStorage>
  bool
  Incoming<RoutingPolicy, PartsStorage>::do_receive_msg(
    Part& part) throw(ZmqErrorType)
  {
    assert(part.valid());
    recv_msg(src_, part.msg());
    const bool more = has_more(src_);
    if (receive_observer_)
    {
      receive_observer_->on_receive_part(part.msg(), more);
    }
    return more;
  }

  template <class RoutingPolicy, class PartsStorage>
  template <typename T>
  Incoming<RoutingPolicy, PartsStorage>&
  Incoming<RoutingPolicy, PartsStorage>::operator>> (T& t)
  throw(NoSuchPartError)
  {
    Multipart::check_has_part(cur_extract_idx_);
    get(Multipart::parts()[cur_extract_idx_++].msg(), t, binary_mode_);
    return *this;
  }

  template <class RoutingPolicy, class PartsStorage>
  void
  Incoming<RoutingPolicy, PartsStorage>::check_is_terminal() const
  throw(MessageFormatError)
  {
    if (!is_terminal_)
    {
      std::ostringstream ss;
      ss <<
        "Receiving multipart: Has more messages after part " <<
        Multipart::size() << ", but must be terminal";
      throw MessageFormatError(ss.str());
    }
  }

  template <class RoutingPolicy, class PartsStorage>
  void
  Incoming<RoutingPolicy, PartsStorage>::append_message_data(
    zmq::message_t& message, std::vector<char>& area) const
  {
    std::vector<char>::size_type sz = area.size();
    area.resize(sz + message.size());
    std::copy(static_cast<char*>(message.data()),
              static_cast<char*>(message.data()) + message.size(),
              area.begin() + sz
    );
  }

  template <class RoutingPolicy, class PartsStorage>
  bool
  Incoming<RoutingPolicy, PartsStorage>::receive_one()
  throw(ZmqErrorType, MessageFormatError)
  {
    Part* const cur_part = ContainerType::next();
    if (!cur_part)
    {
      std::ostringstream ss;
      ss <<
        "Receiving multipart: "
        "Cannot allocate storage for next part, "
        "size " << size() << " reached";
      throw MessageFormatError(ss.str());
    }

    const bool more = do_receive_msg(*cur_part);

    ZMQMESSAGE_LOG_STREAM << "Incoming received "
      << cur_part->msg().size() << " bytes: "
      << ZMQMESSAGE_STRING_CLASS((const char*)cur_part->msg().data(),
        std::min(cur_part->msg().size(), static_cast<size_t>(256)))
      << ", has more = " << more << ZMQMESSAGE_LOG_TERM;
    return more;
  }

  template <class RoutingPolicy, class PartsStorage>
  void
  Incoming<RoutingPolicy, PartsStorage>::validate(
    const char* part_names[],
    size_t part_names_length, bool strict)
    throw (MessageFormatError)
  {
    if (size() < part_names_length)
    {
      std::ostringstream ss;
      ss <<
        "Validating received multipart: "
        "No more messages after " << part_names[size() - 1] <<
        "(" << size() << "), expected " <<
        (strict ? "exactly" : "at least") << " " << part_names_length;
      throw MessageFormatError(ss.str());
    }
    if (strict && size() > part_names_length)
    {
      std::ostringstream ss;
      ss <<
        "Validating received multipart: "
        "Have received " << size() << " parts, while expected "
        "exactly " << part_names_length;
      throw MessageFormatError(ss.str());
    }
  }

  template <class RoutingPolicy, class PartsStorage>
  Incoming <RoutingPolicy, PartsStorage>&
  Incoming<RoutingPolicy, PartsStorage>::receive(
    size_t parts, const char* part_names[],
    size_t part_names_length, bool check_terminal)
    throw (MessageFormatError, ZmqErrorType)
  {
    RoutingPolicy::receive_routing(src_);
    RoutingPolicy::log_routing_received();

    for (size_t i = 0, init_parts = size(); i < parts; ++i)
    {
      bool more = receive_one();
      const char* const part_name =
        (i < part_names_length) ? part_names[i] : "<unnamed>";

      if (i < parts - 1 && !more)
      {
        is_terminal_ = true;
        std::ostringstream ss;
        ss <<
          "Receiving multipart: "
          "No more messages after " << part_name <<
          "(" << (init_parts + i) << "), expected more";
        throw MessageFormatError(ss.str());
      }
      if (i == parts - 1 && more)
      {
        is_terminal_= false;
        if (check_terminal)
        {
          std::ostringstream ss;
          ss <<
            "Receiving multipart: "
            "Has more messages after " << part_name <<
            "(" << (init_parts + i) << "), expected no more messages";
          throw MessageFormatError(ss.str());
        }
      }
      if (i == parts - 1 && !more)
      {
        is_terminal_ = true;
      }
    }
    return *this;
  }

  template <class RoutingPolicy, class PartsStorage>
  Incoming <RoutingPolicy, PartsStorage>&
  Incoming<RoutingPolicy, PartsStorage>::receive_all(
    size_t min_parts, const char* part_names[], size_t part_names_length)
    throw (MessageFormatError, ZmqErrorType)
  {
    receive(min_parts, part_names, part_names_length, false);

    while (!is_terminal_)
    {
      is_terminal_ = !receive_one();
    }

    return *this;
  }

  template <class RoutingPolicy, class PartsStorage>
  Incoming <RoutingPolicy, PartsStorage>&
  Incoming<RoutingPolicy, PartsStorage>::receive_up_to(
    size_t min_parts,
    const char* part_names[], size_t max_parts)
    throw (MessageFormatError, ZmqErrorType)
  {
    receive(min_parts, part_names, false);

    for (size_t n = min_parts; n < max_parts && !is_terminal_; ++n)
    {
      is_terminal_ = !receive_one();
    }

    return *this;
  }

  template <class RoutingPolicy, class PartsStorage>
  int
  Incoming<RoutingPolicy, PartsStorage>::fetch_tail(
    std::vector<char>& area, const char* delimiter) throw (ZmqErrorType)
  {
    assert(size() > 0);
    append_message_data(Multipart::parts()[size()-1].msg(), area);
    if (is_terminal_)
    {
      return 1;
    }

    size_t delim_sz = (delimiter) ? ::strlen(delimiter) : 0;

    int num_messages = 1;
    for (bool more = has_more(src_); more; ++num_messages)
    {
      if (delim_sz)
      {
        std::vector<char>::size_type sz = area.size();
        area.resize(sz + delim_sz);
        std::copy(delimiter, delimiter + delim_sz, area.begin() + sz);
      }
      Part data_buff;
      more = do_receive_msg(data_buff);
      append_message_data(data_buff.msg(), area);
    }
    return num_messages;
  }

  template <class RoutingPolicy, class PartsStorage>
  int
  Incoming<RoutingPolicy, PartsStorage>::drop_tail() throw(ZmqErrorType)
  {
    if (is_terminal_)
    {
      return 0;
    }

    Part data_buff;
    bool more = false;
    int num_messages = 0;
    if (!size()) //we haven't received anything
    {
      if (!try_recv_msg(src_, data_buff.msg(), ZMQ_NOBLOCK))
      {
        return 0;
      }
      more = has_more(src_);
      if (receive_observer_)
      {
        receive_observer_->on_receive_part(data_buff.msg(), more);
      }
      num_messages = 1;
    }
    else
    {
      more = has_more(src_);
    }

    for (; more; ++num_messages)
    {
      more = do_receive_msg(data_buff);
    }
    return num_messages;
  }

  template <class RoutingPolicy, class PartsStorage>
  Incoming<RoutingPolicy, PartsStorage>&
  Incoming<RoutingPolicy, PartsStorage>::operator>> (
    zmq::message_t& msg) throw(NoSuchPartError)
  {
    Multipart::check_has_part(cur_extract_idx_);
    copy_msg(msg, Multipart::parts()[cur_extract_idx_++].msg());
    return *this;
  }

  template<typename RoutingPolicy, typename PartsStorage>
  Incoming<RoutingPolicy, PartsStorage>&
  Skip(Incoming<RoutingPolicy, PartsStorage>& in)
  {
    in.check_has_part(in.cur_extract_idx_);
    ++in.cur_extract_idx_;
    return in;
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

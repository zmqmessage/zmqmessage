/**
 * @file MultipartContainer.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_MULTIPARTCONTAINER_HPP_
#define ZMQMESSAGE_MULTIPARTCONTAINER_HPP_

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/MetaTypes.hpp>
#include <zmqmessage/Multipart.hpp>
#include <zmqmessage/PartsStorage.hpp>

namespace ZmqMessage
{
  namespace Private
  {
    template <typename PartsStorage>
    class MultipartContainer : public PartsStorage, public Multipart
    {
    private:
      using PartsStorage::size_;

      /**
       * Empty parts storage. Used in PartsStorage::detach
       */
      MultipartContainer() :
        Multipart(PartsStorage::parts_addr(), &size_)
      {}

      //to call MultipartContainer() from detach
      template <typename Allocator>
      friend class ::ZmqMessage::DynamicPartsStorage;

      friend class ::ZmqMessage::Sink;

    protected:
      explicit
      MultipartContainer(typename PartsStorage::StorageArg arg) :
        PartsStorage(arg),
        Multipart(PartsStorage::parts_addr(), &size_)
      {}

      inline
      Part*
      next()
      {
        return PartsStorage::next();
      }

    public:
      virtual ~MultipartContainer()
      {
      }
    };
  }
}

#endif /* ZMQMESSAGE_MULTIPARTCONTAINER_HPP_ */

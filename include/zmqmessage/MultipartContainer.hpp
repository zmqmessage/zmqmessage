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
       * Empty parts storage. Used in release
       */
      MultipartContainer() :
        Multipart(PartsStorage::parts_addr(), &size_)
      {}

      static const bool is_dynamic =
        Private::IsDynamicPartsStorage<PartsStorage>::value;

      friend class ::ZmqMessage::Sink;

    protected:
      explicit
      MultipartContainer(size_t capacity) :
        PartsStorage(capacity),
        Multipart(PartsStorage::parts_addr(), &size_)
      {}

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

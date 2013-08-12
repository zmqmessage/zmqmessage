/**
 * @file PartsStorage.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_PARTSSTORAGE_HPP_
#define ZMQMESSAGE_PARTSSTORAGE_HPP_

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/NonCopyable.hpp>
#include <zmqmessage/Part.hpp>

namespace ZmqMessage
{
  namespace Private
  {
    struct ZMQMESSAGE_DLL_LOCAL RoutingStorageTag {};
  }

  /**
   * @brief Allocates ZMQ messages on stack, fixed capacity.
   */
  template <size_t N>
  class ZMQMESSAGE_DLL_PUBLIC StackPartsStorage : private Private::NonCopyable
  {
  public:
    static const size_t default_capacity = N;

    struct ZMQMESSAGE_DLL_LOCAL Empty {};

    typedef Empty StorageArg;
    static const Empty default_storage_arg;

  protected:
    Part parts_[N];
    size_t size_;
  private:
    Part* parts_p_; //store pointer to stack-allocated storage

  protected:

    ZMQMESSAGE_DLL_LOCAL
    explicit
    StackPartsStorage(Private::RoutingStorageTag ignored) :
      size_(0), parts_p_(&(parts_[0]))
    {}

    ZMQMESSAGE_DLL_LOCAL
    explicit
    StackPartsStorage(StorageArg ignored) :
      size_(0), parts_p_(&(parts_[0]))
    {}

    /**
     * @return pointer to next part, 0 if limit is reached
     */
    ZMQMESSAGE_DLL_LOCAL
    inline
    Part*
    next()
    {
      return (size_ < N) ? &(parts_[size_++]) : static_cast<Part*>(0);
    }

    ZMQMESSAGE_DLL_LOCAL
    inline
    Part**
    parts_addr()
    {
      return &parts_p_;
    }
  };

  /**
   * @brief Use external buffer with allocated ZMQ messages
   */
  class ZMQMESSAGE_DLL_PUBLIC ExternalPartsStorage :
  private Private::NonCopyable
  {
  public:
    static const size_t default_capacity = 0;

    class ZMQMESSAGE_DLL_LOCAL Buffer
    {
    private:
      Part* const parts_;
      const size_t size_;

      friend class ExternalPartsStorage;

    public:
      Buffer(Part* parts, size_t size) :
        parts_(parts), size_(size)
      {}
    };

    typedef Buffer StorageArg;
    static const Buffer default_storage_arg;

  protected:
    size_t size_;

  private:
    Part* parts_;
    const size_t limit_;

  protected:
    /**
     * Undefined, since Routing storage on external buffer is not supported
     */
    ZMQMESSAGE_DLL_LOCAL
    explicit
    ExternalPartsStorage(Private::RoutingStorageTag ignored);

    explicit
    ExternalPartsStorage(Buffer buffer) :
      size_(0), parts_(buffer.parts_), limit_(buffer.size_)
    {}

    /**
     * @return pointer to next part, 0 if size is reached
     */
    inline
    Part*
    next()
    {
      return (size_ < limit_) ? &(parts_[size_++]) : static_cast<Part*>(0);
    }

    inline
    Part**
    parts_addr()
    {
      return &parts_;
    }
  };

  /**
   * @brief Allocates ZMQ messages using allocator, grows automatically
   */
  template <typename Allocator>
  class ZMQMESSAGE_DLL_PUBLIC DynamicPartsStorage :
    private Allocator, private Private::NonCopyable
  {
  public:
    typedef Allocator PartsAllocatorType;
    static const size_t default_capacity = ZMQMESSAGE_DYNAMIC_DEFAULT_CAPACITY;

    typedef size_t StorageArg;
    static const StorageArg default_storage_arg = default_capacity;

    /**
     * Detach heap-allocated Multipart message
     * owning all the parts of this storage.
     * After this operation, current object owns no message parts.
     * New object uses the same allocator.
     */
    Multipart*
    detach();

  protected:
    Part* parts_;
    size_t capacity_;
    size_t size_;

    /**
     * @return pointer to next part
     */
    ZMQMESSAGE_DLL_LOCAL
    Part*
    next();

    ZMQMESSAGE_DLL_LOCAL
    explicit
    DynamicPartsStorage(Private::RoutingStorageTag tag);

    ZMQMESSAGE_DLL_LOCAL
    explicit
    DynamicPartsStorage(StorageArg capacity);

    /**
     * Empty storage, no allocations
     */
    ZMQMESSAGE_DLL_LOCAL
    DynamicPartsStorage();

    ZMQMESSAGE_DLL_LOCAL
    ~DynamicPartsStorage();

    ZMQMESSAGE_DLL_LOCAL
    inline
    Part**
    parts_addr()
    {
      return &parts_;
    }
  };
}

#endif /* ZMQMESSAGE_PARTSSTORAGE_HPP_ */

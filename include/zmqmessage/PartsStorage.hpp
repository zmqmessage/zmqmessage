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
  /**
   * @brief Allocates ZMQ messages on stack, fixed capacity.
   */
  template <size_t N>
  class StackPartsStorage : private Private::NonCopyable
  {
  public:
    static const size_t default_capacity = N;
  protected:
    Part parts_[N];
    size_t size_;
  private:
    Part* parts_p_; //store pointer to stack-allocated storage

  protected:
    explicit
    StackPartsStorage(size_t ignored) :
      size_(0), parts_p_(&(parts_[0]))
    {}

    /**
     * @return pointer to next part, 0 if limit is reached
     */
    inline
    Part*
    next()
    {
      return (size_ < N) ? &(parts_[size_++]) : static_cast<Part*>(0);
    }

    inline
    Part**
    parts_addr()
    {
      return &parts_p_;
    }
  };

  /**
   * @brief Allocates ZMQ messages using allocator, grows automatically
   */
  template <typename Allocator = std::allocator<Part> >
  class DynamicPartsStorage : private Allocator, private Private::NonCopyable
  {
  public:
    typedef Allocator PartsAllocatorType;
    static const size_t default_capacity = ZMQMESSAGE_DYNAMIC_DEFAULT_CAPACITY;

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
    Part*
    next();

    explicit
    DynamicPartsStorage(size_t capacity);

    /**
     * Empty storage, no allocations
     */
    DynamicPartsStorage();

    ~DynamicPartsStorage();

    inline
    Part**
    parts_addr()
    {
      return &parts_;
    }
  };

  namespace Private
  {
    template <class T>
    struct IsDynamicPartsStorage
    {
      typedef char Yes;
      typedef struct { char c[2]; } No;

      template <typename U>
      static No check_dynamic(...);

      template <typename U>
      static Yes check_dynamic(typename U::PartsAllocatorType* = 0);

      static const bool value = (sizeof(check_dynamic<T>(0)) == sizeof(Yes));
    };
  }
}

#endif /* ZMQMESSAGE_PARTSSTORAGE_HPP_ */

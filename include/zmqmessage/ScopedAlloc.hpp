/**
 * @file ScopedAlloc.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_SCOPEDALLOC_HPP_
#define ZMQMESSAGE_SCOPEDALLOC_HPP_

#include <zmqmessage/Config.hpp>

namespace ZmqMessage
{
  namespace Private
  {
    /**
     * Scoped allocation of memory block using given allocator type.
     * Memory can be released.
     */
    template <typename Allocator>
    class ZMQMESSAGE_DLL_LOCAL ScopedAlloc : private Allocator
    {
    private:
      typename Allocator::pointer mem_;
      size_t n_;

    public:
      explicit
      ScopedAlloc(size_t n) : mem_(Allocator::allocate(n)), n_(n)
      {}

      ~ScopedAlloc()
      {
        if (mem_)
        {
          Allocator::deallocate(mem_, n_);
        }
      }

      typename Allocator::pointer
      release()
      {
        typename Allocator::pointer m = mem_;
        mem_ = 0;
        n_ = 0;
        return m;
      }

      typename Allocator::pointer
      mem()
      {
        return mem_;
      }
    };
  }
}

#endif /* ZMQMESSAGE_SCOPEDALLOC_HPP_ */

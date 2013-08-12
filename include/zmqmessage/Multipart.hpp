/**
 * @file Multipart.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_MULTIPART_HPP_
#define ZMQMESSAGE_MULTIPART_HPP_

#include <ZmqMessageFwd.hpp>

#include <zmqmessage/NonCopyable.hpp>
#include <zmqmessage/Part.hpp>

namespace ZmqMessage
{
  /**
   * @brief Basic interface to message parts.
   *
   * Does not actually own parts, just provides access to them.
   * Not aware of parts storage scheme.
   */
  class ZMQMESSAGE_DLL_PUBLIC Multipart : private Private::NonCopyable
  {
  private:
    Part** parts_ptr_; //!< non-null
    size_t* size_ptr_; //!< non-null

  protected:
    void
    check_has_part(size_t n) const throw(NoSuchPartError);

    Multipart(Part** parts_ptr, size_t* size_ptr) :
      parts_ptr_(parts_ptr), size_ptr_(size_ptr)
    {
      assert(parts_ptr_);
      assert(size_ptr_);
    }

    inline
    Part*
    parts()
    {
      return *parts_ptr_;
    }

  private:

    friend class Sink;

    friend void
    send(zmq::socket_t&, Multipart&, bool, SendObserver*)
    throw(ZmqErrorType);

  public:
    /**
     * @return number of parts in this multipart
     */
    inline
    size_t
    size() const
    {
      return *size_ptr_;
    }

    /**
     * Get reference to Part by index
     */
    Part&
    operator[](size_t i) throw (NoSuchPartError);

    const Part&
    operator[](size_t i) const throw (NoSuchPartError);

    /**
     * @brief Input iterator to iterate over message parts
     */
    template <typename T>
    class ZMQMESSAGE_DLL_PUBLIC iterator
    {
    public:
      typedef T value_type;
      typedef T& reference;
      typedef T* pointer;
      typedef std::ptrdiff_t difference_type;
      typedef std::input_iterator_tag iterator_category;

    private:
      ZMQMESSAGE_DLL_LOCAL
      iterator(const Multipart& m, bool binary_mode, bool end = false) :
      multipart_(&m), idx_(end ? m.size() : 0),
      binary_mode_(binary_mode)
      {
        set_cur();
      }

      ZMQMESSAGE_DLL_LOCAL
      iterator(const Multipart& m, size_t idx, bool binary_mode) :
        multipart_(&m), idx_(idx), binary_mode_(binary_mode)
      {
        set_cur();
      }

      template <typename U>
      ZMQMESSAGE_DLL_LOCAL
      iterator(const iterator<U>& rhs) :
        multipart_(rhs.multipart_), idx_(rhs.idx),
        binary_mode_(rhs.binary_mode)
      {
        set_cur();
      }

      friend class Multipart;

    public:
      iterator
      operator++()
      {
        ++idx_;
        set_cur();
        return *this;
      }

      iterator
      operator++(int)
      {
        iterator<T> ret_val(*this);
        ++(*this);
        return ret_val;
      }

      const T&
      operator*() const
      {
        return cur_;
      }

      const T*
      operator->() const
      {
        return &cur_;
      }

      bool
      operator==(const iterator<T>& rhs)
      {
        return equal(rhs);
      }

      bool
      operator!=(const iterator<T>& rhs)
      {
        return !equal(rhs);
      }

    private:
      const Multipart* multipart_;
      size_t idx_;
      T cur_;
      bool binary_mode_; //non-const to use in assignment

      ZMQMESSAGE_DLL_LOCAL
      inline
      bool
      end() const
      {
        return idx_ >= multipart_->size();
      }

      ZMQMESSAGE_DLL_LOCAL
      bool
      equal(const iterator<T>& rhs) const
      {
        return (end() && rhs.end()) ||
          (multipart_ == rhs.multipart_ && idx_ == rhs.idx_);
      }

      ZMQMESSAGE_DLL_LOCAL
      void
      set_cur();
    };

    template <typename T>
    friend class Iterator;

  public:
    virtual ~Multipart()
    {
    }

    /**
     * Does this multipart message has a part at given index (and owns it)
     */
    inline
    bool
    has_part(size_t idx) const
    {
      return (size() > idx && (*parts_ptr_)[idx].valid());
    }

    /**
     * Obtain iterator that yields values of type T.
     * @param binary_mode: how to convert message part to T:
     * interprete as string data (if binary_mode is false)
     * or interpret as binary data (if true).
     * @tparam T must be explicitly specified:
     * @code
     * it = multipart.begin<std::string>();
     * @endcode
     */
    template <typename T>
    inline
    iterator<T>
    begin(bool binary_mode = false) const
    {
      return iterator<T>(*this, binary_mode);
    }

    /**
     * Obtain iterator that yields values of default string type.
     */
    inline
    iterator<ZMQMESSAGE_STRING_CLASS>
    beginstr() const
    {
      return begin<ZMQMESSAGE_STRING_CLASS>();
    }

    /**
     * Obtain iterator positioned to message part at specified index
     * @param pos index, counts from 0.
     * @param binary_mode: how to convert message part to non-string type:
     * interprete as string data (if binary_mode is false)
     * or interpret as binary data (if true).
     */
    template <typename T>
    inline
    iterator<T>
    iter_at(size_t pos, bool binary_mode = false) const
    {
      return iterator<T>(*this, pos, binary_mode);
    }

    /**
     * End iterator that yields values of type T.
     */
    template <typename T>
    inline
    iterator<T>
    end() const
    {
      return iterator<T>(*this, false, true);
    }

    /**
     * End iterator that yields values of default string type.
     */
    inline
    iterator<ZMQMESSAGE_STRING_CLASS>
    endstr() const
    {
      return end<ZMQMESSAGE_STRING_CLASS>();
    }

    /**
     * Release (disown) message part at specified index.
     * @return invalid message if such message is not owned by Multipart
     */
    Part
    release(size_t i);

    inline
    std::auto_ptr<zmq::message_t>
    release_ptr(size_t i)
    {
      Part part = release(i);
      std::auto_ptr<zmq::message_t> ptr(new zmq::message_t());
      ptr->move(&(part.msg()));
      return ptr;
    }
  };
}

#endif /* ZMQMESSAGE_MULTIPART_HPP_ */

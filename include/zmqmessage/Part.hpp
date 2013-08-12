/**
 * @file Part.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_PART_HPP_
#define ZMQMESSAGE_PART_HPP_

#include "zmq.hpp"

namespace ZmqMessage
{
  /**
   * ZMQ Message with copy semantics.
   * When object is copied it moves message contents to its copy!
   * After copying the initial object becomes invalid.
   * Objects may be stored by value in STL containers.
   */
  class ZMQMESSAGE_DLL_PUBLIC Part
  {
  private:
    zmq_msg_t msg_;
    bool valid_; //has meaningful content

    ZMQMESSAGE_DLL_LOCAL
    inline
    void
    init_empty()
    {
      int rc = zmq_msg_init (&msg_);
      if (rc != 0)
        throw error_t ();
    }

  public:
    inline
    explicit
    Part(bool valid = true) : valid_(valid)
    {
      init_empty();
    }

    inline
    explicit
    Part(size_t size_) : valid_(true)
    {
      int rc = zmq_msg_init_size (&msg_, size_);
      if (rc != 0)
        throw error_t ();
    }

    inline
    Part(void *data_, size_t size_, zmq::free_fn *ffn_,
      void *hint_ = NULL) : valid_(true)
    {
      int rc = zmq_msg_init_data (&msg_, data_, size_, ffn_, hint_);
      if (rc != 0)
        throw error_t ();
    }

    inline
    bool
    valid() const
    {
      return valid_;
    }

    inline
    void
    mark_invalid()
    {
      valid_ = false;
    }

    inline
    ~Part()
    {
      zmq_msg_close (&msg_);
      valid_ = false;
    }

    inline
    Part(const Part& part) :
      valid_(part.valid_)
    {
      init_empty();
      Part& src = const_cast<Part&>(part);
      move(src);
    }

    inline
    zmq::message_t&
    msg()
    {
      return *static_cast<zmq::message_t*>(static_cast<void*>(&msg_));
    }

    /**
     * zmq::message_t does not support const-correctness,
     * so we need non-const object where actually it's used as const.
     */
    inline
    zmq::message_t&
    msg() const
    {
      return const_cast<Part*>(this)->msg();
    }

    inline
    void
    copy(Part& src)
    {
      if (src.valid_)
      {
        zmq_msg_copy(&msg_, &src.msg_);
        valid_ = true;
      }
    }

    inline
    void
    move(Part& src)
    {
      if (src.valid_)
      {
        zmq_msg_move(&msg_, &src.msg_);
        valid_ = true;
        src.valid_ = false;
      }
      else
      {
        valid_ = false; //like src
      }
    }

    inline
    void
    move(zmq::message_t& src)
    {
      zmq_msg_move(&msg_, static_cast<zmq_msg_t*>(static_cast<void*>(&src)));
      valid_ = true;
    }

    inline
    operator zmq::message_t& ()
    {
      return msg();
    }

    inline
    operator zmq::message_t& () const
    {
      return msg();
    }

    friend
    inline
    bool
    operator==(const Part& lhs, const Part& rhs)
    {
      return &lhs == &rhs; //same object
    }

    inline
    Part&
    operator=(const Part& rhs) //move ownership
    {
      move(const_cast<Part&>(rhs));
      return *this;
    }
  };

}

#endif /* ZMQMESSAGE_PART_HPP_ */

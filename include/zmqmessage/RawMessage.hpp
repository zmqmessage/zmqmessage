/**
 * @file RawMessage.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_RAWMESSAGE_HPP_
#define ZMQMESSAGE_RAWMESSAGE_HPP_

namespace ZmqMessage
{
  /**
   * @brief Describes memory region with raw data
   *
   * When inserted in Outgoing, a new message part will be created.
   * Deleter is ZMQ free function.
   * If deleter = 0, Outgoing will hold COPY of this memory region.
   * Otherwise, no copying will be performed,
   * the memory will be deleted by deleter
   */
  struct RawMessage
  {
    union
    {
      void* ptr;
      const void* cptr;
    } data;
    size_t sz;
    zmq::free_fn* deleter;

    /**
     * Take ownership on the memory region specified by data_p and sz_p.
     * ZMQ will delete it with deleter_p when sent.
     */
    inline RawMessage(void* data_p, size_t sz_p, zmq::free_fn* deleter_p) :
      sz(sz_p), deleter(deleter_p)
    {
      data.ptr = data_p;
    }

    /**
     * When sending, we will copy memory region specified by data_p and sz_p.
     */
    inline RawMessage(const void* data_p, size_t sz_p) :
      sz(sz_p), deleter(0)
    {
      data.cptr = data_p;
    }
  };
}

#endif /* ZMQMESSAGE_RAWMESSAGE_HPP_ */

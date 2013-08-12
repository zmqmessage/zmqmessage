/**
 * @file RawMessage.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
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
  struct ZMQMESSAGE_DLL_PUBLIC RawMessage
  {
    union
    {
      void* ptr;
      const void* cptr;
    } data;
    size_t sz;
    zmq::free_fn* deleter;
    void* hint;

    /**
     * Take ownership on the memory region specified by data_p and sz_p.
     * ZMQ will delete it with deleter_p when sent.
     */
    inline RawMessage(void* data_p, size_t sz_p,
      zmq::free_fn* deleter_p, void* hint_p = 0) :
      sz(sz_p), deleter(deleter_p), hint(hint_p)
    {
      data.ptr = data_p;
    }

    /**
     * When sending, we will copy memory region specified by data_p and sz_p.
     */
    inline RawMessage(const void* data_p, size_t sz_p) :
      sz(sz_p), deleter(0), hint(0)
    {
      data.cptr = data_p;
    }
  };
}

#endif /* ZMQMESSAGE_RAWMESSAGE_HPP_ */

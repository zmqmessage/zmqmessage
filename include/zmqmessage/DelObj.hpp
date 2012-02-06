/**
 * @file DelObj.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_DELOBJ_HPP_
#define ZMQMESSAGE_DELOBJ_HPP_

namespace ZmqMessage
{
  namespace Private
  {
    template<class T>
    void
    del_obj(T* obj)
    {
      delete obj;
    }

    template<class T>
    void
    del_obj_not_null(T* obj)
    {
      if (obj != 0)
      {
        delete obj;
      }
    }
  }
}

#endif /* ZMQMESSAGE_DELOBJ_HPP_ */

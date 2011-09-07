/**
 * @file DelObj.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <nifigase@gmail.com>, et al.
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

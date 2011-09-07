/**
 * @file NonCopyable.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <nifigase@gmail.com>, et al.
 *
 */

#ifndef ZMQMESSAGE_NONCOPYABLE_HPP_
#define ZMQMESSAGE_NONCOPYABLE_HPP_

namespace ZmqMessage
{
  namespace Private
  {
    /**
     * To make your class NonCopyable inherit from NonCopyable class
     * using private access mode, i.e.
     * class some_type : private NonCopyable
     * {
     *   ...
     * };
     */
    class NonCopyable
    {
    protected:
      NonCopyable() {}
      ~NonCopyable() {}
    private:
      NonCopyable(NonCopyable const&);
      NonCopyable& operator=(NonCopyable const&);
    };
  }
}

#endif // ZMQMESSAGE_NONCOPYABLE_HPP_


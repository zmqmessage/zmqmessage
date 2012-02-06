/**
 * @file MetaTypes.hpp
 * @copyright Copyright (c) 2010-2011 Phorm, Inc.
 * @copyright GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
 * @author Andrey Skryabin <andrew@zmqmessage.org>, et al.
 *
 */

#ifndef ZMQMESSAGE_METATYPES_HPP_
#define ZMQMESSAGE_METATYPES_HPP_

namespace ZmqMessage
{
  /**
   * @namespace ZmqMessage::Private
   * @brief For internal usage
   *   to determine the proper way to pass a particular data type.
   */
  namespace Private
  {
    template <bool, class T = void>
    struct EnableIf {};

    template <class T>
    struct EnableIf<true, T> { typedef T type; };

    template <bool, class T = void>
    struct DisableIf {};

    template <class T>
    struct DisableIf<false, T> { typedef T type; };

    template <class T>
    struct IsStr
    {
      typedef char Yes;
      typedef struct { char c[2]; } No;

      template <typename U, size_t (U::*LengthFn)() const = &U::length>
      struct LengthTraits;
      template <typename U, const char* (U::*DataFn)() const = &U::data>
      struct DataTraits;

      template <typename U>
      static No check_str(...);

      template <typename U>
      static Yes check_str(LengthTraits<U>*, DataTraits<U>*);

      static const bool value = (sizeof(check_str<T>(0, 0)) == sizeof(Yes));
    };

    template <class T>
    struct IsRaw
    {
      typedef char Yes;
      typedef struct { char c[2]; } No;

      template <typename U>
      static No check_mark(...);

      template <typename U>
      static Yes check_mark(typename U::raw_mark* = 0);

      static const bool value = (sizeof(check_mark<T>(0)) == sizeof(Yes));
    };
  }
}

/**
 * @def ZMQMESSAGE_BINARY_TYPE
 * Declares particular type as 'binary'. It will always be sent and received
 * in binary mode, independent of current stream state.
 * See \ref zm_modes modes docs for details.
 */
#define ZMQMESSAGE_BINARY_TYPE(type_name) \
namespace ZmqMessage \
{ \
  namespace Private \
  { \
    template <> \
    struct IsRaw<type_name> \
    { \
    static const bool value = true; \
    }; \
  } \
}

/**
 * @def ZMQMESSAGE_TEXT_TYPE
 * Declares particular type as 'text'. It will always be sent and received
 * in text mode, independent of current stream state.
 * See \ref zm_modes modes docs for details.
 */
#define ZMQMESSAGE_TEXT_TYPE(type_name) \
namespace ZmqMessage \
{ \
  namespace Private \
  { \
    template <> \
    struct IsRaw<type_name> \
    { \
    static const bool value = false; \
    }; \
  } \
}



#endif /* ZMQMESSAGE_METATYPES_HPP_ */

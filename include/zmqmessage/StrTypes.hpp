/**
 * @file StrTypes.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_STRTYPES_HPP_
#define ZMQMESSAGE_STRTYPES_HPP_

namespace ZmqMessage
{
  /**
   * @namespace ZmqMessage::Private
   * @brief For internal usage, nothing to look at here.
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

//    template <class T>
//    struct IsRaw
//    {
//      typedef char Yes;
//      typedef struct { char c[2]; } No;
//
//      template <typename U>
//      static No check_mark(...);
//
//      template <typename U>
//      static Yes check_mark(typename U::raw_mark* = 0);
//
//      static const bool value = (sizeof(check_mark<T>(0)) == sizeof(Yes));
//    };

  }
}

#endif /* ZMQMESSAGE_STRTYPES_HPP_ */

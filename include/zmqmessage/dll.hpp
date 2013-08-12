/**
 * @file dll.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_DLL_HPP_
#define ZMQMESSAGE_DLL_HPP_

/**
 * @def ZMQMESSAGE_BUILDING_DLL
 * Should be externally defined by build system
 * if we are building shared object.
 * It will turn on visibility control with macros
 * @ref ZMQMESSAGE_DLL_LOCAL and @ref ZMQMESSAGE_DLL_PUBLIC
 */

/**
 * @def ZMQMESSAGE_DLL_LOCAL
 * Macro for controlling functions and types visibility in shared library.
 * Entities declared with this macro will not be visible
 * in resulting shared library.
 */

/**
 * @def ZMQMESSAGE_DLL_PUBLIC
 * Macro for controlling functions and types visibility in shared library.
 * Entities declared with this macro will be visible
 * in resulting shared library.
 */

#if defined _WIN32 || defined __CYGWIN__
  #ifdef ZMQMESSAGE_BUILDING_DLL
    #ifdef __GNUC__
      #define ZMQMESSAGE_DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define ZMQMESSAGE_DLL_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define ZMQMESSAGE_DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define ZMQMESSAGE_DLL_PUBLIC __declspec(dllimport)
    #endif
  #endif
  #define ZMQMESSAGE_DLL_LOCAL
#else
  #if __GNUC__ >= 4 && defined(ZMQMESSAGE_BUILDING_DLL)
    #define ZMQMESSAGE_DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define ZMQMESSAGE_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define ZMQMESSAGE_DLL_PUBLIC
    #define ZMQMESSAGE_DLL_LOCAL
  #endif
#endif

#endif /* ZMQMESSAGE_DLL_HPP_ */

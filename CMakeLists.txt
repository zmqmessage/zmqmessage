# Copyright (c) 2010-2011 Phorm, Inc.
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
# Author: Andrey Skryabin <andrew@zmqmessage.org>, et al.

cmake_minimum_required(VERSION 2.6)

SET(CMAKE_CXX_FLAGS "-Wall -fvisibility=hidden -DZMQMESSAGE_BUILDING_DLL"
  CACHE
  STRING "Flags used by the compiler during all builds"
  )
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG"
  CACHE
  STRING "Flags used by the compiler during release with debug info builds"
  )
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG"
  CACHE
  STRING "Flags used by the compiler during release builds"
  )
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g"
  CACHE
  STRING "Flags used by the compiler during debug builds"
  )

SET(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS}
  CACHE
  STRING "Flags used by the compiler during all C builds"
  )
SET(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
  CACHE
  STRING "Flags used by the compiler during release with debug info C builds"
  )
SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE}
  CACHE
  STRING "Flags used by the compiler during release C builds"
  )
SET(CMAKE_C_FLAGS_DEBUG  ${CMAKE_CXX_FLAGS_DEBUG}
  CACHE
  STRING "Flags used by the compiler during debug builds"
  )

project(ZMQMESSAGE)

# additional cmake modules location path
set(
  CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules
  CACHE
  PATH "CMake modules search path"
)

# additional configuration
set(
  ZMQMESSAGE_CONFIGURATION_HEADER
  ""
  CACHE
  PATH "User-supplied header with macro definitions which will be included. Leave empty for default build"
)

# all compiled executables and libraries
# will be created inside "${BUILD_DIR}/bin"
# and "${BUILD_DIR}/lib" directories
set(
  CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/bin
)

set(
  CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/lib
)

# projects common include directory path(s)
set(
  ZMQMESSAGE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  CACHE
  INTERNAL ""
  )

find_package(ZeroMQ)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)
add_subdirectory(examples)

add_custom_target(doc
  doxygen && echo "To browse docs: open ${CMAKE_CURRENT_SOURCE_DIR}/doc/html/index.html"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating documentation")

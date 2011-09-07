# Copyright (c) 2010-2011 Phorm, Inc.
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
# Author: Andrey Skryabin <nifigase@gmail.com>, et al.

cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_FLAGS_DEBUG  "-g -Wall"
  CACHE
  STRING "Flags used by the compiler during debug builds"
  )



# project(ZMQMESSAGE)


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

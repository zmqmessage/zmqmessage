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

# set(CPACK_GENERATOR "RPM")
# SET(CPACK_SOURCE_GENERATOR "TGZ")
# include(CPack)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)
add_subdirectory(examples)


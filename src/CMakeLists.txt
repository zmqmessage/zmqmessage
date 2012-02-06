project(ZMQMESSAGE)

# projects library name
set(TARGET_NAME zmqmessage)

find_package(ZeroMQ)

set(ZMQMESSAGE_SOURCE_FILES
  ZmqMessage.cpp
  ZmqTools.cpp
  )

# projects include directory
include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
  )

# link projects object files
# into an executable named
# ${TARGET_NAME}
add_library(${TARGET_NAME}
  SHARED
  ${ZMQMESSAGE_SOURCE_FILES}
  )

# link ${TARGET_NAME} target with the
# following libraries
target_link_libraries(${TARGET_NAME}
  ${ZEROMQ_LIBRARIES}
  )

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)

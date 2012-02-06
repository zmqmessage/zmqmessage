# Copyright (c) 2010-2011 Phorm, Inc.
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
# Author: Andrey Skryabin <nifigase@gmail.com>, et al.

project(ZMQMESSAGE)

# projects library name
set(TARGET_NAME zmqmessage)

set(ZMQMESSAGE_SOURCE_FILES
  ZmqMessage.cpp
  )

# projects include directory
include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
  )

if (${ZMQMESSAGE_CONFIGURATION_HEADER})
  set_target_properties(${TARGET_NAME}
    PROPERTIES COMPILE_DEFINITIONS "ZMQMESSAGE_CONFIGURATION_HEADER=${ZMQMESSAGE_CONFIGURATION_HEADER}"
  )
endif()

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

include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
  )

add_executable(zsort
  zsort.cpp
  )

add_executable(zbisort
  zbisort.cpp
  )

target_link_libraries(zsort
  pthread
  zmqmessage
  )

target_link_libraries(zbisort
  pthread
  zmqmessage
  )


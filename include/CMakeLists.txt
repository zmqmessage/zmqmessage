FILE(
  GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${files}
  DESTINATION include)

FILE(
  GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/zmqmessage/*.hpp")
INSTALL(FILES ${files}
  DESTINATION include/zmqmessage)


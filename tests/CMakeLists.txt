include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
)

add_executable(SimpleTest
  SimpleTest.cpp
)

target_link_libraries(SimpleTest
 pthread
 zmqmessage
)

add_test(SimpleTest
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SimpleTest)

add_executable(SimpleTestHO
  SimpleTest.cpp
)
set_target_properties(SimpleTestHO
  PROPERTIES COMPILE_DEFINITIONS "HEADERONLY"
)
target_link_libraries(SimpleTestHO
 pthread
 ${ZEROMQ_LIBRARIES}
 #no zmqmessage
)
add_test(SimpleTestHO
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SimpleTestHO)

add_executable(PerfTest
  PerfTest.cpp
)

target_link_libraries(PerfTest
 pthread
 zmqmessage
)
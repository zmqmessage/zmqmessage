include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
  )

MACRO(OneFileExample)
  FOREACH(exname ${ARGN})
    ADD_EXECUTABLE(${exname}
      ./${exname}.cpp
      )
    TARGET_LINK_LIBRARIES(${exname}
      pthread
      zmqmessage)
  ENDFOREACH(exname)
ENDMACRO(OneFileExample)

OneFileExample(
  zsort
  zbisort
  zqueue
  zserialize
  )


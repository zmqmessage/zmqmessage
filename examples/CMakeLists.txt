# Copyright (c) 2010-2011 Phorm, Inc.
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
# Author: Andrey Skryabin <nifigase@gmail.com>, et al.

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


# Copyright (c) 2010-2011 Phorm, Inc.
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html 
# Author: Andrey Skryabin <andrew@zmqmessage.org>, et al.

FILE(
  GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${files}
  DESTINATION include)

FILE(
  GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/zmqmessage/*.hpp")
INSTALL(FILES ${files}
  DESTINATION include/zmqmessage)


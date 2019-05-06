# Copyright (C) 2007-2016 Hypertable, Inc.
#
# This file is part of Hypertable.
#
# Hypertable is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or any later version.
#
# Hypertable is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hypertable. If not, see <http://www.gnu.org/licenses/>
#

# - Find Thrift (a cross platform RPC lib/tool)
# This module defines
#  THRIFT_VERSION, version string of ant if found
#  THRIFT_INCLUDE_PATHS, where to find Thrift headers
#  THRIFT_LIBRARIES, Thrift libraries
#  THRIFT_FOUND, If false, do not try to use ant

exec_program(env ARGS thrift -version OUTPUT_VARIABLE THRIFT_VERSION
             RETURN_VALUE Thrift_RETURN)
			 
SET_DEPS(
	NAME "THRIFT" 
	REQUIRED TRUE 
	LIB_PATHS /usr/local/lib/thrift
	INC_PATHS ${HT_DEPENDENCY_INCLUDE_DIR}/thrift
			  /usr/local/include/thrift
			  /usr/include/thrift
			  /opt/local/include/thrift
	STATIC libthrift.a libthriftnb.a  libthriftz.a 
	SHARED thrift thriftnb thriftz
	INCLUDE Thrift.h
)


if (THRIFT_VERSION MATCHES "^Thrift version" AND EVENT_LIBRARIES_SHARED AND THRIFT_LIBRARIES_SHARED)

  exec_program(${CMAKE_SOURCE_DIR}/bin/src-utils/ldd.sh
               ARGS ${THRIFT_LIBRARIES_SHARED}
               OUTPUT_VARIABLE LDD_OUT
               RETURN_VALUE LDD_RETURN)

  if (LDD_RETURN STREQUAL "0")
    string(REGEX MATCH "[ \t](/[^ ]+/libssl\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libgssapi_krb5\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkrb5\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libcom_err\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libk5crypto\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libcrypto\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkrb5support\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libz\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkeyutils\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Thrift_LIB_DEPENDENCIES "${Thrift_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    
    string(REPLACE " " ";" Thrift_LIB_DEPENDENCIES ${Thrift_LIB_DEPENDENCIES})
  endif ()

else ()
  set(THRIFT_FOUND FALSE)
  if (NOT EVENT_LIBRARIES_SHARED)
    message(STATUS "    libevent is required for thrift broker support")
  endif ()
endif ()

if (THRIFT_FOUND)
  if (NOT Thrift_FIND_QUIETLY)
    message("       compiler: ${THRIFT_VERSION}")
  endif ()
  string(REPLACE "\n" " " THRIFT_VERSION ${THRIFT_VERSION})
  string(REPLACE " " ";" THRIFT_VERSION ${THRIFT_VERSION})
  list(GET THRIFT_VERSION -1 THRIFT_VERSION)
  
  set(ThriftBroker_IDL_DIR ${HYPERTABLE_SOURCE_DIR}/src/cc/ThriftBroker)

  if (THRIFT_FOUND)
	  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHT_WITH_THRIFT")
	  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHT_WITH_THRIFT")
  endif ()
  if (BOOST_NO_CXX11_SMART_PTR)
	  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBOOST_NO_CXX11_SMART_PTR")
	  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_NO_CXX11_SMART_PTR")
  endif ()
  

  HT_INSTALL_LIBS(lib ${THRIFT_LIBRARIES_SHARED} ${Thrift_LIB_DEPENDENCIES})

else ()
  message(STATUS "Thrift compiler/libraries NOT found. "
          "Thrift support will be disabled (${Thrift_RETURN}, "
          "${THRIFT_INCLUDE_PATHS}, ${THRIFT_LIBRARIES_SHARED})")
endif ()

  


# Copy C++ Thrift files
install(DIRECTORY ${THRIFT_INCLUDE_PATHS} DESTINATION include USE_SOURCE_PERMISSIONS)

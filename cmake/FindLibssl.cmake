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

# - Find Libssl
# Find the libssl includes and library
#
#  SSL_INCLUDE_DIR      - where to find ssl.h, etc.
#  SSL_LIBRARIES        - libssl libraries
#  Libssl_LIB_DEPENDENCIES - List of libraries when using libssl.
#  SSL_FOUND            - True if libssl found.


SET_DEPS(
	NAME "SSL" 
	REQUIRED TRUE 
	LIB_PATHS /usr/local/ssl/lib
	INC_PATHS /usr/local/ssl/include
	STATIC libssl.a libcrypto.a 
	SHARED ssl crypto
	INCLUDE openssl/ssl.h openssl/crypto.h
)



if (SSL_FOUND)
	
	# if ssl with dso "dl" required
	#exec_program(nm ARGS ${SSL_LIBRARIES}
	#			OUTPUT_VARIABLE LDD_OUT
	#			RETURN_VALUE LDD_RETURN)
	#if (LDD_RETURN STREQUAL "gg")
	#	string(REGEX MATCH "dlfcn_globallookup" dummy ${LDD_OUT})
	#	message(STATUS ${dummy})
	#	if (dummy)
	#		set(SSL_LIBRARIES ${SSL_LIBRARIES} dl)
	#	endif ()
	#	message(STATUS ${SSL_LIBRARIES})
	#endif ()
			   
  exec_program(${CMAKE_SOURCE_DIR}/bin/src-utils/ldd.sh
               ARGS ${SSL_LIBRARIES_SHARED}
               OUTPUT_VARIABLE LDD_OUT
               RETURN_VALUE LDD_RETURN)
  if (LDD_RETURN STREQUAL "0")
    string(REGEX MATCH "[ \t](/[^ ]+/libgssapi_krb5\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkrb5\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libcom_err\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libk5crypto\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkrb5support\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
    string(REGEX MATCH "[ \t](/[^ ]+/libkeyutils\\.[^ \n]+)" dummy ${LDD_OUT})
    set(Libssl_LIB_DEPENDENCIES "${Libssl_LIB_DEPENDENCIES} ${CMAKE_MATCH_1}")
  endif ()

  set(TC_TRY_OUT "foo")
  try_run(TC_CHECK TC_CHECK_BUILD
          ${HYPERTABLE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
          ${HYPERTABLE_SOURCE_DIR}/cmake/CheckLibssl.cc
          CMAKE_FLAGS 
		  INCLUDE_DIRECTORIES ${SSL_INCLUDE_PATHS}
          LINK_LIBRARIES ${SSL_LIBRARIES_SHARED}
          RUN_OUTPUT_VARIABLE TC_TRY_OUT)
  if (TC_CHECK_BUILD AND NOT TC_CHECK STREQUAL "0")
    message(STATUS "${TC_TRY_OUT}")
    message(FATAL_ERROR "Please fix the libssl installation and try again.")
  endif ()
  message("       version: ${TC_TRY_OUT}")
  
  if(Libssl_LIB_DEPENDENCIES)
	  # Install dependencies
	  string(REPLACE " " ";" LIB_DEPENDENCIES_LIST ${Libssl_LIB_DEPENDENCIES})
	  HT_INSTALL_LIBS(lib ${LIB_DEPENDENCIES_LIST})
  endif ()
endif ()


if(SSL_LIBRARIES_SHARED)
  HT_INSTALL_LIBS(lib ${SSL_LIBRARIES_SHARED})
endif()
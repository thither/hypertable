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

# - Find Jemalloc
# Find the native Jemalloc includes and library
#
#  JEMALLOC_INCLUDE_PATHS - where to find Jemalloc.h, etc.
#  JEMALLOC_LIBRARIES_SHARED     - List of libraries when using Jemalloc.
#  JEMALLOC_LIBRARIES_STATIC     - List of libraries when using Jemalloc.
#  JEMALLOC_FOUND         - True if Jemalloc found.

SET_DEPS(
	NAME "JEMALLOC" 
	REQUIRED FALSE 
	LIB_PATHS 
	INC_PATHS 
	STATIC libjemalloc.a
	SHARED jemalloc
	INCLUDE jemalloc/jemalloc.h
)

if (JEMALLOC_FOUND)
  try_run(TC_CHECK TC_CHECK_BUILD
          ${HYPERTABLE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
          ${HYPERTABLE_SOURCE_DIR}/cmake/CheckJemalloc.cc
          CMAKE_FLAGS 
		      INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE_PATHS}
          LINK_LIBRARIES ${JEMALLOC_LIBRARIES_SHARED}
          OUTPUT_VARIABLE TC_TRY_OUT)

  #message("tc_check build: ${TC_CHECK_BUILD}")
  #message("tc_check: ${TC_CHECK}")
  #message("tc_version: ${TC_TRY_OUT}")
  if (TC_CHECK_BUILD AND NOT TC_CHECK STREQUAL "0")
    string(REGEX REPLACE ".*\n(Jemalloc .*)" "\\1" TC_TRY_OUT ${TC_TRY_OUT})
    message(STATUS "${TC_TRY_OUT}")
    message(FATAL_ERROR "Please fix the jemalloc installation and try again.")
    set(Jemalloc_LIBRARIES)
  endif ()
  string(REGEX REPLACE ".*\n([0-9]+[^\n]+).*" "\\1" TC_VERSION ${TC_TRY_OUT})
  if (NOT TC_VERSION MATCHES "^[0-9]+.*")
    set(TC_VERSION "unknown -- make sure it's 1.1+")
  endif ()
  message("       version: ${TC_VERSION}")
endif ()


if(JEMALLOC_LIBRARIES_SHARED)
  HT_INSTALL_LIBS(lib ${JEMALLOC_LIBRARIES_SHARED})
endif()

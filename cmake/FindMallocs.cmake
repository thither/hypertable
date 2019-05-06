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


if (USE_GLIBC_MALLOC OR (
              CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" OR
              CMAKE_SYSTEM_PROCESSOR STREQUAL "i586" OR
              CMAKE_SYSTEM_PROCESSOR STREQUAL "i686"))
  set(JEMALLOC_LIBRARIES_SHARED )
  set(JEMALLOC_LIBRARIES_STATIC )


elseif (USE_JEMALLOC)
  find_package(Jemalloc)
  if (JEMALLOC_FOUND)
    set(MALLOC_LIBRARIES_SHARED ${JEMALLOC_LIBRARIES_SHARED})
    set(MALLOC_LIBRARIES_STATIC ${JEMALLOC_LIBRARIES_STATIC})
    set(MALLOC_INCLUDE_PATHS    ${JEMALLOC_INCLUDE_PATHS})
  else ()
    message(FATAL_ERROR "Unable to use jemalloc: library not found")
  endif ()


elseif (USE_HOARD)
  find_package(Hoard)
  if (Hoard_FOUND)
    set(MALLOC_LIBRARIES_SHARED ${HOARD_LIBRARIES_SHARED})
    set(MALLOC_LIBRARIES_STATIC ${HOARD_LIBRARIES_STATIC})
    set(MALLOC_INCLUDE_PATHS    ${HOARD_INCLUDE_PATHS})
  else ()
    message(FATAL_ERROR "Unable to use hoard malloc: library not found")
  endif ()


else()  # TCMALLOC default if found
  find_package(Tcmalloc)
  if (TCMALLOC_FOUND)
    if (TCMALLOC_LIBRARIES_SHARED MATCHES "tcmalloc_minimal")
      SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTCMALLOC_MINIMAL")
	    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTCMALLOC_MINIMAL")
    else ()
      SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTCMALLOC")
      SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTCMALLOC")
    endif ()

    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    set(MALLOC_LIBRARIES_SHARED ${TCMALLOC_LIBRARIES_SHARED})
    set(MALLOC_LIBRARIES_STATIC ${TCMALLOC_LIBRARIES_STATIC})
    set(MALLOC_INCLUDE_PATHS    ${TCMALLOC_INCLUDE_PATHS})
  endif ()
  
endif ()



if(MALLOC_LIBRARIES_SHARED)
  message(STATUS "Using MALLOC: ${MALLOC_LIBRARIES_SHARED} ${TCMALLOC_LIBRARIES_STATIC} ")
  HT_INSTALL_LIBS(lib ${MALLOC_LIBRARIES_SHARED})
else ()
  message(STATUS "Using MALLOC: GLIBC_MALLOC ")
endif ()


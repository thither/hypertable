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

# This is used by many thrift targets to guard against make clean
macro(HT_GLOB var)
  file(GLOB_RECURSE ${var} ${ARGN})
  if (NOT ${var}) # make clean would remove generated files
    # add a dummy target here
    set(${var} "${var}_DUMMY")
  endif ()
  #message(STATUS "${var}: ${${var}}")
endmacro()

if (PACKAGE_THRIFTBROKER)
  set(HT_COMPONENT_INSTALL true)
  message(STATUS "Prepare for ThriftBroker only installation")
endif ()



macro(HT_GET_SONAME var fpath)
  exec_program(${CMAKE_SOURCE_DIR}/bin/src-utils/soname.sh ARGS ${fpath}
               OUTPUT_VARIABLE SONAME_OUT RETURN_VALUE SONAME_RETURN)
  set(${var})

  if (SONAME_RETURN STREQUAL "0")
    set(${var} ${SONAME_OUT})
  endif ()

  if (NOT ${var})
    get_filename_component(${var} ${fpath} NAME)
  endif ()

  if (HT_CMAKE_DEBUG)
    message("SONAME: ${fpath} -> ${${var}}")
  endif ()

  # check if the library is prelinked, if so, warn
  exec_program(env ARGS objdump -h ${fpath} OUTPUT_VARIABLE ODH_OUT
               RETURN_VALUE ODH_RETURN)
  if (ODH_RETURN STREQUAL "0")
    string(REGEX MATCH "prelink_undo" match ${ODH_OUT})
    if (match)
      message("WARNING: ${fpath} is prelinked, RPMs may require --nomd5")
    endif ()
  endif ()
endmacro()

# This is a workaround for install() which always preserves symlinks
macro(HT_INSTALL_LIBS dest)
 if (NOT HT_COMPONENT_INSTALL)
  if (INSTALL_EXCLUDE_DEPENDENT_LIBS)
    message(STATUS "Not installing dependent libraries")
  else ()
    foreach(fpath ${ARGN})
      if (NOT ${fpath} MATCHES "(NOTFOUND|\\.a)$")
        if (HT_CMAKE_DEBUG)
          message(STATUS "install copy: ${fpath}")
        endif ()
        HT_GET_SONAME(soname ${fpath})
        configure_file(${fpath} "${dest}/${soname}" COPYONLY)
        install(FILES "${CMAKE_BINARY_DIR}/${dest}/${soname}" DESTINATION ${dest})
      else ()
         message(STATUS "Problem installing ${fpath} soname=${soname} to ${dest}")
      endif ()
    endforeach()
  endif ()
 endif ()
endmacro()

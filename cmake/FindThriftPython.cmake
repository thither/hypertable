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

# - Find PYTHON5
# This module defines
#  PYTHONTHRIFT_FOUND, If false, do not try to use ant
#  THIRFT_PY_PATH

if (THRIFT_SOURCE_DIR)
  exec_program("find ${THRIFT_SOURCE_DIR}/lib/py/build/ -name Thrift.py"
                OUTPUT_VARIABLE PYTHONTHRIFT_OUT
                RETURN_VALUE PYTHONTHRIFT_RETURN)
  if (PYTHONTHRIFT_RETURN STREQUAL "0")
    string(REPLACE /Thrift.py "" THIRFT_PY_PATH ${PYTHONTHRIFT_OUT})
  endif()
endif()

if (THIRFT_PY_PATH)
  list(GET PYTHON_EXECUTABLES 0 py)
  exec_program(env ARGS  PYTONPATH=${THIRFT_PY_PATH} ${py} -c'import thrift'
               OUTPUT_VARIABLE PYTHONTHRIFT_OUT
               RETURN_VALUE PYTHONTHRIFT_RETURN)
               
  if (PYTHONTHRIFT_RETURN STREQUAL "0")
    message(STATUS "Found thrift Python")
    set(PYTHONTHRIFT_FOUND TRUE)

  elseif(LANG_PY)
    message(FATAL_ERROR "Thrift Python lib is bad: ${PYTHONTHRIFT_OUT}")
  endif ()

elseif(LANG_PY)
  message(FATAL_ERROR "Thrift Python lib files not found, Looked for ${THRIFT_SOURCE_DIR}/lib/py/build/*/Thrift.py ${PERLTHRIFT_OUT}")
endif ()


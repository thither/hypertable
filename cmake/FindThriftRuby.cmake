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

# - Find Ruby Thrift module
# This module defines
#  RUBYTHRIFT_FOUND, If false, do not Ruby w/ thrift


if (THRIFT_SOURCE_DIR AND EXISTS ${THRIFT_SOURCE_DIR}/lib/rb/lib/thrift/client.rb)

  exec_program(env ARGS ruby -I${THRIFT_SOURCE_DIR}/lib/rb/lib -r thrift -e 0 
                OUTPUT_VARIABLE RUBYTHRIFT_OUT
                RETURN_VALUE RUBYTHRIFT_RETURN)

  if (RUBYTHRIFT_RETURN STREQUAL "0")
    message(STATUS "Found thrift Ruby")
    set(RUBYTHRIFT_FOUND TRUE)
  
    elseif(LANG_RB)
    message(FATAL_ERROR "Thrift Ruby lib is bad: ${RUBYTHRIFT_OUT}")
  endif ()

elseif(LANG_RB)
  message(FATAL_ERROR "Thrift Ruby lib files not found, Looked for ${THRIFT_SOURCE_DIR}/lib/rb/lib/thrift/client.rb")
endif ()

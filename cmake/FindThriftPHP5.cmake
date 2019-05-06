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

# - Find PHP5
# This module defines
#  PHPTHRIFT_FOUND, If false, do not try to use ant


if (THRIFT_SOURCE_DIR AND EXISTS ${THRIFT_SOURCE_DIR}/lib/php/lib/Base/TBase.php) 
  message(STATUS "Found thrift PHP")
  set(PHPTHRIFT_FOUND TRUE)
  
elseif(LANG_PHP)
  message(FATAL_ERROR "Thrift PHP lib files not found, Looked for ${THRIFT_SOURCE_DIR}/lib/php/lib/Base/TBase.php")
endif ()



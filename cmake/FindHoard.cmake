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

# - Find Hoard
# Find the native Hoard includes and library
#
#  HOARD_LIBRARIES_STATIC   - List of libraries when using Hoard.
#  HOARD_LIBRARIES_SHARED   - List of libraries when using Hoard.
#  HOARD_FOUND       - True if Hoard found.


SET_DEPS(
	NAME "HOARD" 
	REQUIRED FALSE 
	LIB_PATHS 
	INC_PATHS 
	STATIC libhoard.a
	SHARED hoard
	INCLUDE hoard.h
)

if(HOARD_LIBRARIES_SHARED)
  HT_INSTALL_LIBS(lib ${HOARD_LIBRARIES_SHARED})
endif()
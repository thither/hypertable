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

# - Find Qfs
# Find the native Qfs includes and library
#
#  QFS_INCLUDE_DIR - where to find Kfs.h, etc.
#  QFS_LIBRARIES_SHARED   - List of libraries when using Kfs.
#  QFS_FOUND       - True if Kfs found.


SET_DEPS(
	NAME "QFS" 
	REQUIRED FALSE
	LIB_PATHS /opt/qfs/lib/static /opt/qfs/lib $ENV{HOME}/src/qfs/build/lib/static
	INC_PATHS /opt/qfs/include /opt/local/include $ENV{HOME}/src/qfs/build/include
	SHARED qfs_client qfs_io qfs_common qfs_qcdio qfs_qcrs qfskrb Jerasure gf_complete
	INCLUDE kfs/KfsClient.h
)


if(NOT QFS_FOUND AND FSBROKER_QFS)
    message(FATAL_ERROR "Could NOT find QFS libraries")
endif ()


if(QFS_LIBRARIES_SHARED)
	HT_INSTALL_LIBS(lib ${QFS_LIBRARIES_SHARED})
endif()
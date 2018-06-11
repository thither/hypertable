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

# - Find Mapr
# Find the native Mapr includes and library
#
#  Mapr_INCLUDE_DIR - where to find Mapr.h, etc.
#  Mapr_LIBRARIES   - List of libraries when using Mapr.
#  Mapr_FOUND       - True if Mapr found.

HT_FASTLIB_SET(
	NAME "MAPR" 
	LIB_PATHS /opt/mapr/lib  
			  $ENV{HADOOP_HOME}/lib/native
	INC_PATHS /$ENV{HADOOP_HOME}/include 
			  /opt/mapr/hadoop/hadoop-0.20.2/src/c++/libhdfs
	STATIC libhdfs.a 
	SHARED hdfs 
	INCLUDE hdfs.h
)

if (MAMPR_INCLUDE_DIR AND MAPR_LIBRARIES)
  set(Mapr_FOUND TRUE)
  
  if (jvm_LIB)
	set(MAPR_LIBRARIES ${MAPR_LIBRARIES} ${jvm_LIB})
  else ()
	if (FSBROKER_MAPR)
		message(FATAL_ERROR "Could NOT find jvm_LIB for MAPR libraries")
	endif ()
	set(Mapr_FOUND FALSE)
	set(MAPR_LIBRARIES)
  endif ()

else ()
   if (FSBROKER_MAPR)
      message(FATAL_ERROR "Could NOT find MAPR libraries")
   endif ()
   set(Mapr_FOUND FALSE)
   set(MAPR_LIBRARIES)
endif ()

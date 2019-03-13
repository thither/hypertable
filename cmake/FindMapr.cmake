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
#  MAPR_INCLUDE_DIR - where to find Mapr.h, etc.
#  MAPR_LIBRARIES   - List of libraries when using Mapr.
#  MAPR_FOUND       - True if Mapr found.

SET_DEPS(
	NAME "MAPR" 
	LIB_PATHS /opt/mapr/lib  
						$ENV{HADOOP_HOME}/lib/native
						${HADOOP_LIB_PATH}/lib/native
	INC_PATHS /$ENV{HADOOP_HOME}/include 
			  		/opt/mapr/hadoop/hadoop-0.20.2/src/c++/libhdfs
						${HADOOP_INCLUDE_PATH}/include 
	STATIC libhdfs.a 
	SHARED hdfs 
	INCLUDE hdfs.h
)

if (MAPR_INCLUDE_PATHS AND 
		((MAPR_LIBRARIES_SHARED AND JAVA_LIBRARIES_SHARED) OR (MAPR_LIBRARIES_STATIC AND JAVA_LIBRARIES_STATIC)))

	HT_INSTALL_LIBS(lib ${MAPR_LIBRARIES_SHARED})
	set(MAPR_LIBRARIES_SHARED ${MAPR_LIBRARIES_SHARED} ${JAVA_LIBRARIES_SHARED})
	set(MAPR_LIBRARIES_STATIC ${MAPR_LIBRARIES_STATIC} ${JAVA_LIBRARIES_STATIC})

else ()
   if (FSBROKER_MAPR)
	 		if (NOT JAVA_LIBRARIES_SHARED)
		 		message(FATAL_ERROR "Could NOT find jvm_LIB for MAPR libraries")
	 		endif ()
      message(FATAL_ERROR "Could NOT find MAPR libraries")
   endif ()
   set(MAPR_FOUND FALSE)
   set(MAPR_LIBRARIES_SHARED)
   set(MAPR_LIBRARIES_STATIC)
endif ()


if(JEMALLOC_LIBRARIES_SHARED)
	HT_INSTALL_LIBS(lib ${MAPR_LIBRARIES_SHARED})
endif()
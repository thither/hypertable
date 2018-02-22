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

# - Find Maven (a java build tool)
# This module defines
#  MAVEN_VERSION version string of maven if found
#  MAVEN_FOUND, If false, do not try to use maven


if (NOT JAVA_INCLUDE_PATH)
    if (NOT $ENV{JAVA_INCLUDE_PATH})
		set(JAVA_INCLUDE_PATH $ENV{JAVA_HOME}/include )
	else ()
		set(JAVA_INCLUDE_PATH $ENV{JAVA_INCLUDE_PATH} )
	endif ()
endif ()
  
exec_program(env ARGS java -version OUTPUT_VARIABLE JAVARE_OUT
             RETURN_VALUE JAVARE_RETURN)

if (JAVARE_RETURN STREQUAL "0")
   set(JAVARE_FOUND TRUE) 
   string(REPLACE ";" " " JAVARE_OUT ${JAVARE_OUT})
   string(REPLACE "\n" ";" JAVARE_OUT ${JAVARE_OUT})
   list(GET JAVARE_OUT 0 JAVARE_OUT)
   message(STATUS "Java Runtime Environment: ${JAVARE_OUT}")

else ()
  message(STATUS " Java Runtime Environment: not found")
  set(JAVARE_FOUND FALSE)
endif ()


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



if (NOT JAVA_INCLUDE_PATH OR JAVA_INCLUDE_PATH STREQUAL "")
    if (NOT $ENV{JAVA_INCLUDE_PATH})
		set(JAVA_INCLUDE_PATH "$ENV{JAVA_HOME}/include" )
	else ()
		set(JAVA_INCLUDE_PATH "$ENV{JAVA_INCLUDE_PATH}" )
	endif ()
endif ()
  
exec_program(env ARGS java -version OUTPUT_VARIABLE JAVARE_OUT
             RETURN_VALUE JAVARE_RETURN)

if (JAVARE_RETURN STREQUAL "0")
   set(JAVARE_FOUND TRUE) 
   string(REPLACE ";" " " JAVARE_OUT ${JAVARE_OUT})
   string(REPLACE "\n" ";" JAVARE_OUT ${JAVARE_OUT})
   list(GET JAVARE_OUT 0 JAVARE_OUT)
   message(STATUS "Java RE: ${JAVARE_OUT}")

else ()
   message(STATUS "Java RE: not found")
   set(JAVARE_FOUND FALSE)
endif ()



exec_program(env ARGS javac -version OUTPUT_VARIABLE JAVAC_OUT
             RETURN_VALUE JAVAC_RETURN)

if (JAVAC_RETURN STREQUAL "0")
  message(STATUS "Java Compiler: ${JAVAC_OUT}")
  
  string(REGEX MATCH "1[0-9]\\..*" JAVAC_VERSION ${JAVAC_OUT})
  if (NOT JAVAC_VERSION)
    string(REGEX MATCH "1\\.[6-9]\\..*" JAVAC_VERSION ${JAVAC_OUT})
    if (NOT JAVAC_VERSION)
		  message(STATUS "    Expected JDK 1.6 or greater. Skipping Java build. (${JAVAC_OUT})")
		  set(SKIP_JAVA_BUILD TRUE)
	  endif ()
  endif ()
	
else ()
  message(STATUS "Java Compiler: not found")
  set(SKIP_JAVA_BUILD TRUE)
endif ()


SET_DEPS(
  NAME "JAVA" 
  
	LIB_PATHS $ENV{JAVA_HOME}/jre/lib/amd64
	          $ENV{JAVA_HOME}/jre/lib/amd64/server  
	          $ENV{JAVA_HOME}/lib/server
	          $ENV{JAVA_HOME}/lib
	INC_PATHS $ENV{JAVA_HOME}/include
	# STATIC libjvm.a libjava.a libverify.a # libjawt.a 
	SHARED  jvm java verify # jawt(requires Xrender, Xtst, Xi)
	INCLUDE jni.h
)

if(JAVA_LIBRARIES_SHARED)
  HT_INSTALL_LIBS(lib ${JAVA_LIBRARIES_SHARED})
endif()

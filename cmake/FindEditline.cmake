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

# Find the Editline includes and libraries
# This module defines
# EDITLINE_INCLUDE_DIR, where to find editline/readline.h, etc.
# EDITLINE_LIBRARIES, the libraries needed to use editline.
# EDITLINE_FOUND, If false, do not try to use editline.
# also defined, but not for general use are
# EDITLINE_LIBRARY, NCURSES_LIBRARY


SET_DEPS(
	NAME "TINFOW" 
	LIB_PATHS 
	INC_PATHS 
	STATIC libncursesw.a libtinfow.a 
	SHARED ncursesw tinfow
	INCLUDE ncurses.h termcap.h
)
if (NOT TINFOW_LIBRARIES_SHARED AND NOT TINFOW_LIBRARIES_STATIC)
	SET_DEPS(
		NAME "TINFOW" 
		REQUIRED TRUE 
		LIB_PATHS 
		INC_PATHS 
		STATIC libncurses.a libtinfo.a
		SHARED ncurses tinfo
		INCLUDE ncurses.h termcap.h
	)
endif ()


SET_DEPS(
	NAME "EDITLINE" 
	REQUIRED TRUE 
	LIB_PATHS 
	INC_PATHS 
	STATIC libedit.a 
	SHARED edit
	INCLUDE editline/readline.h
)

try_run(EDITLINE_CHECK EDITLINE_CHECK_BUILD
        ${HYPERTABLE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
        ${HYPERTABLE_SOURCE_DIR}/cmake/CheckEditline.cc
        CMAKE_FLAGS 
        INCLUDE_DIRECTORIES ${EDITLINE_INCLUDE_PATHS}
        LINK_LIBRARIES ${EDITLINE_LIBRARIES_SHARED} ${TINFOW_LIBRARIES_SHARED} ${BDB_LIBRARIES_SHARED} pthread
        OUTPUT_VARIABLE EDITLINE_TRY_OUT)
if (EDITLINE_CHECK_BUILD STREQUAL "FALSE")
	message(STATUS "${EDITLINE_TRY_OUT}")
	message(FATAL_ERROR "Please fix the Editline installation and try again.  Make sure you build libedit with --enable-widec!")
endif ()


if(EDITLINE_LIBRARIES_SHARED)
  HT_INSTALL_LIBS(lib ${EDITLINE_LIBRARIES_SHARED} ${TINFOW_LIBRARIES_SHARED})
endif()
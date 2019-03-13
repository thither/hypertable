#
# Copyright (C) 2007-2016 Hypertable, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 3 of
# the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#


set(fsbroker_SRCS
fsbroker.cc
CommandInterpreter.cc
)

# ht_fsbroker - command interpreter
ADD_UTIL_TARGET(
	NAME ht_fsbroker
	SRCS ${fsbroker_SRCS}
  TARGETS HyperCommon HyperComm HyperFsBroker Hypertable
  SHARED ${EDITLINE_LIBRARIES_SHARED}
  STATIC ${EDITLINE_LIBRARIES_STATIC}
)



set(SRC_DIR "${HYPERTABLE_SOURCE_DIR}/src/cc/Tools/client/fsbroker")
set(DST_DIR "${HYPERTABLE_BINARY_DIR}/src/cc/Tools/client/fsbroker")

add_custom_command(SOURCE ${HYPERTABLE_SOURCE_DIR}/tests/data/words.gz
    COMMAND gzip ARGS -d < ${HYPERTABLE_SOURCE_DIR}/tests/data/words.gz
                         > ${DST_DIR}/words
    OUTPUT ${DST_DIR}/words)
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${DST_DIR}/words") 

# fsTest
ADD_TEST_TARGET(
	NAME HyperFsBroker-shell
	SRCS fsTest.cc FsTestThreadFunction.cc
	TARGETS Hypertable HyperCommon HyperComm HyperFsBroker
	EXEC_DEPS ${DST_DIR}/words
)
configure_file(${SRC_DIR}/fsTest.golden ${DST_DIR}/fsTest.golden COPYONLY)


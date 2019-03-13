#
# Copyright (C) 2007-2016 Hypertable, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or any later version.
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

set(CMAKE_CXX_FLAGS "-DHAVE_NETINET_IN_H ${CMAKE_CXX_FLAGS}")

# IF .thirft instruction change with version it is prefered .thrift by versions
if( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thriftgen-${THRIFT_VERSION})
	execute_process(COMMAND  mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/thriftgen-${THRIFT_VERSION})
endif ()
if( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thriftgen-${THRIFT_VERSION}/gen-cpp)
	execute_process(COMMAND  thrift -r --gen cpp 
							 -o ${CMAKE_CURRENT_SOURCE_DIR}/thriftgen-${THRIFT_VERSION}/
							 ${CMAKE_CURRENT_SOURCE_DIR}/Hql.thrift)
	# set current gen-cpp/ included path				 
	execute_process(COMMAND rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp)	
	execute_process(COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/thriftgen-${THRIFT_VERSION}/gen-cpp
	                              ${CMAKE_CURRENT_SOURCE_DIR}/)			 
endif ()

# lib HyperThrift - no other hypertable libs dependencies
ADD_LIB_TARGET(
	NAME HyperThrift 
	SRCS 	gen-cpp/Client_constants.cpp 
				gen-cpp/ClientService.cpp 
				gen-cpp/Client_types.cpp
				gen-cpp/Hql_constants.cpp 
				gen-cpp/HqlService.cpp 
				gen-cpp/Hql_types.cpp
	SHARED ${THRIFT_LIBRARIES_SHARED} ${EVENT_LIBRARIES_SHARED}
	STATIC ${THRIFT_LIBRARIES_STATIC} ${EVENT_LIBRARIES_STATIC}
)

# lib HyperThriftExtentions
ADD_LIB_TARGET(
	NAME HyperThriftExtentions 
	SRCS SerializedCellsReader.cc SerializedCellsWriter.cc ThriftHelper.cc
	TARGETS Hypertable
)

# lib HyperThriftConfig
ADD_LIB_TARGET(
	NAME HyperThriftConfig 
	SRCS Config.cc
	TARGETS HyperThrift Hypertable
)

	
# htThriftBroker
ADD_EXEC_TARGET(
	NAME htThriftBroker
	SRCS ThriftBroker.cc MetricsHandler.cc
  TARGETS HyperThriftExtentions HyperThriftConfig HyperAppHelper
  SHARED ${CEPH_LIBRARIES_SHARED}
  STATIC ${CEPH_LIBRARIES_STATIC}
)


### ZLIB TRANSPORT TESTS
# Basic regression test
ADD_TEST_TARGET(
	NAME ThriftClientZ-cpp
	SRCS tests/client_test.cc
	TARGETS HyperThrift HyperThriftExtentions HyperCommon Hypertable 
	ARGS zlib
	PRE_CMD env bash ${INSTALL_DIR}/bin/ht-start-test-servers.sh --clear --opt--thrift-transport=zlib
)
# regression test for SerializedCellsWriter/SerializedCellsReader
ADD_TEST_TARGET(
	NAME ThriftClientZ-cpp-Serialized
	SRCS tests/client_test.cc
	TARGETS HyperThrift HyperThriftExtentions HyperCommon Hypertable 	
	ARGS zlib
	POST_CMD env bash ${INSTALL_DIR}/bin/ht-start-test-servers.sh --clear
)

### FRAMED TRANSPORT TESTS
# Basic regression test
ADD_TEST_TARGET(
	NAME ThriftClient-cpp
	SRCS tests/client_test.cc
	TARGETS HyperThrift HyperThriftExtentions HyperCommon Hypertable 
	ARGS framed
)
# regression test for SerializedCellsWriter/SerializedCellsReader
ADD_TEST_TARGET(
	NAME ThriftClient-cpp-Serialized
	SRCS tests/serialized_test.cc
	TARGETS HyperThrift HyperThriftExtentions HyperCommon Hypertable 
	ARGS framed
)

if (NOT HT_COMPONENT_INSTALL OR PACKAGE_THRIFTBROKER)
  install(FILES Client.h ThriftHelper.h SerializedCellsFlag.h SerializedCellsReader.h SerializedCellsWriter.h Client.thrift Hql.thrift
          DESTINATION include/ThriftBroker)
  install(DIRECTORY gen-cpp DESTINATION include/ThriftBroker)
endif ()

if (PACKAGE_THRIFTBROKER)
  file(GLOB SCRIPTS ${CMAKE_SOURCE_DIR}/bin/*broker.sh ${CMAKE_SOURCE_DIR}/bin/stop-servers.sh ${CMAKE_SOURCE_DIR}/bin/random-wait.sh)
  install(PROGRAMS ${SCRIPTS} DESTINATION bin)
  set(CPACK_PACKAGE_NAME "hypertable-thriftbroker-only")
  include(PackageHelper)
endif ()

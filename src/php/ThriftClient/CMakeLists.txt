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



add_custom_target(ThriftGenPHP ALL)
add_custom_command(
  TARGET 		ThriftGenPHP
  COMMAND   thrift
  ARGS      -r --gen php:classmap
            -o ${HYPERTABLE_BINARY_DIR}/src/php/ThriftClient
            ${ThriftBroker_IDL_DIR}/Hql.thrift
  DEPENDS   ${ThriftBroker_IDL_DIR}/Client.thrift
            ${ThriftBroker_IDL_DIR}/Hql.thrift
  COMMENT   "ThriftPhpGen"
)

         

if (NOT HT_COMONENT_INSTALL OR PACKAGE_THRIFTBROKER)

  install(DIRECTORY ${THRIFT_SOURCE_DIR}/lib/php/lib/ DESTINATION lib/php/Thrift)

  file(GLOB PHPFILES ${HYPERTABLE_SOURCE_DIR}/src/php/*.php)
  install(FILES ${PHPFILES} DESTINATION lib/php)

  install(DIRECTORY ${HYPERTABLE_BINARY_DIR}/src/php/ThriftClient/gen-php DESTINATION lib/php)


  add_test(ThriftClient-php 
            env PHPTHRIFT_ROOT=${INSTALL_DIR}/lib/php
            php ${CMAKE_CURRENT_SOURCE_DIR}/client_test.php)
            
endif ()

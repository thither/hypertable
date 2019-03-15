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

set(thriftbroker_SRCS
  CommandInterpreter.cc
  thriftbroker.cc
)

# ht_thriftbroker - command interpreter
ADD_UTIL_TARGET(
	NAME ht_thriftbroker
	SRCS ${thriftbroker_SRCS}
  TARGETS HyperThriftConfig 
  SHARED ${EDITLINE_LIBRARIES_SHARED}
  STATIC ${EDITLINE_LIBRARIES_STATIC}
)

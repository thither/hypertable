#
# Copyright (C) 2007-2014 Hypertable, Inc.
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

# ht_ssh - Mulit-host ssh automation tool

ADD_UTIL_TARGET(
	NAME ht_ssh
	SRCS ht_ssh.cc SshSocketHandler.cc SshThreadsCallbacks.cc SshOutputCollector.cc
  TARGETS HyperCommon HyperComm
  SHARED  ${SSH_LIBRARIES_SHARED} ${SSP_LIBRARIES_SHARED}
  STATIC ${SSH_LIBRARIES_STATIC} ${SSP_LIBRARIES_STATIC}
)
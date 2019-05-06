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

# ht_prune_tsv - drops lines with date string older than window
ADD_UTIL_TARGET(
	NAME ht_prune_tsv
	SRCS prune_tsv.cc
  TARGETS HyperCommon HyperComm
)

set(SRC_DIR "${HYPERTABLE_SOURCE_DIR}/src/cc/Tools/prune_tsv")
set(DST_DIR "${HYPERTABLE_BINARY_DIR}/src/cc/Tools/prune_tsv")

add_test(prune-tsv env INSTALL_DIR=${INSTALL_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh)
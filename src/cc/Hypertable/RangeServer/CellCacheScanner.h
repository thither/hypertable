/** -*- c++ -*-
 * Copyright (C) 2008 Doug Judd (Zvents, Inc.)
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
 * License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef HYPERTABLE_CELLCACHESCANNER_H
#define HYPERTABLE_CELLCACHESCANNER_H

#include "CellCache.h"
#include "CellListScanner.h"
#include "ScanContext.h"


namespace Hypertable {

  /**
   * Provides a scanning interface to a CellCache.
   */
  class CellCacheScanner : public CellListScanner {
  public:
    CellCacheScanner(CellCachePtr &cellcache, ScanContextPtr &scan_ctx);
    virtual ~CellCacheScanner() { return; }
    virtual void forward();
    virtual bool get(Key &key, ByteString &value);

  private:
    CellCache::CellMap::iterator   m_start_iter;
    CellCache::CellMap::iterator   m_end_iter;
    CellCache::CellMap::iterator   m_cur_iter;
    CellCachePtr                   m_cell_cache_ptr;
    Mutex                         &m_cell_cache_mutex;
    Key                            m_cur_key;
    ByteString                     m_cur_value;
    bool                           m_eos;
    bool                           m_has_start_deletes;
    bool                           m_has_start_row_delete;
    bool                           m_has_start_cf_delete;
    Key                            m_start_deletes[2];
    DynamicBuffer                  m_start_delete_buf;
    bool                           m_keys_only;
  };
}

#endif // HYPERTABLE_CELLCACHESCANNER_H


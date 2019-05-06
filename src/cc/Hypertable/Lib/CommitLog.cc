/*
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3 of the
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

/// @file
/// Definitions for CommitLog.
/// This file contains definitions for CommitLog, a class for creating and
/// appending entries to an edit log.

#include <Common/Compat.h>

#include "CommitLog.h"

#include <Hypertable/Lib/BlockCompressionCodec.h>
#include <Hypertable/Lib/BlockHeaderCommitLog.h>
#include <Hypertable/Lib/CommitLogReader.h>
#include <Hypertable/Lib/CompressorFactory.h>

#include <AsyncComm/Protocol.h>

#include <Common/Checksum.h>
#include <Common/Config.h>
#include <Common/DynamicBuffer.h>
#include <Common/Error.h>
#include <Common/FileUtils.h>
#include <Common/Logger.h>
#include <Common/StringExt.h>
#include <Common/Time.h>
#include <Common/md5.h>

#include <cassert>
#include <chrono>

using namespace Hypertable;
using namespace std;

const char CommitLog::MAGIC_DATA[10] =
    { 'C','O','M','M','I','T','D','A','T','A' };
const char CommitLog::MAGIC_LINK[10] =
    { 'C','O','M','M','I','T','L','I','N','K' };


CommitLog::CommitLog(FilesystemPtr &fs, const string &log_dir, bool is_meta)
  : CommitLogBase(log_dir), m_fs(fs) {
  initialize(log_dir, Config::properties, 0, is_meta);
}

CommitLog::~CommitLog() {
  close();
}

void
CommitLog::initialize(const string &log_dir, PropertiesPtr &props,
                      CommitLogBase *init_log, bool is_meta) {
  string compressor;

  m_log_dir = log_dir;
  m_cur_fragment_num = 0;
  m_needs_roll = false;
  m_replication = -1;

  if (is_meta)
    m_replication = props->get_i32("Hypertable.Metadata.Replication");
  else
    m_replication = props->get_i32("Hypertable.RangeServer.Data.DefaultReplication");

  SubProperties cfg(props, "Hypertable.CommitLog.");

  HT_TRY("getting commit log properites",
    m_max_fragment_size = cfg.get_i64("RollLimit");
    compressor = cfg.get_str("Compressor"));

  m_compressor.reset(CompressorFactory::create_block_codec(compressor));

  boost::trim_right_if(m_log_dir, boost::is_any_of("/"));
 
  m_range_reference_required = props->get_bool("Hypertable.RangeServer.CommitLog.FragmentRemoval.RangeReferenceRequired");

  if (init_log) {
    if (m_range_reference_required)
      m_range_reference_required = init_log->range_reference_required();
    stitch_in(init_log);
    for (const auto frag : m_fragment_queue) {
      if (frag->num >= m_cur_fragment_num)
        m_cur_fragment_num = frag->num + 1;
    }
  }
  else {  // chose one past the max one found in the directory
    uint32_t num;
    std::vector<Filesystem::Dirent> listing;
    m_fs->readdir(m_log_dir, listing);
    for (size_t i=0; i<listing.size(); i++) {
      num = atoi(listing[i].name.c_str());
      if (num >= m_cur_fragment_num)
        m_cur_fragment_num = num + 1;
    }
  }

  if (m_range_reference_required)
    HT_INFOF("Range reference for '%s' is required", m_log_dir.c_str());
  else
    HT_INFOF("Range reference for '%s' is NOT required", m_log_dir.c_str());

  m_fs->mkdirs(m_log_dir);
  create_next_log();

}

int CommitLog::create_next_log() {

  m_cur_fragment_fname = m_log_dir + "/" + m_cur_fragment_num;
  m_smartfd_ptr = Filesystem::SmartFd::make_ptr(
      m_cur_fragment_fname, Filesystem::OPEN_FLAG_OVERWRITE);

  int32_t write_tries = 0;
  try_write_again:

  try {
    m_fs->create(m_smartfd_ptr, -1, m_replication, -1);
    CommitLogBlockStream::write_header(m_fs, m_smartfd_ptr);
    m_cur_fragment_length = CommitLogBlockStream::header_size();
  }
  catch (Exception &e) {
    HT_ERRORF("Problem initializing commit log '%s', try: %d  - %s (%s)",
              m_log_dir.c_str(), write_tries, e.what(), Error::get_text(e.code()));
              
    if(m_fs->retry_write_ok(m_smartfd_ptr, e.code(), &write_tries))
      goto try_write_again;
      
    return e.code();
  }

  return Error::OK;
}


int64_t CommitLog::get_timestamp() {
  return get_ts64();
}

int CommitLog::flush() {
  lock_guard<mutex> lock(m_mutex);
  int error {};

  try {
    if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
      return Error::CLOSED;
    m_fs->flush(m_smartfd_ptr);
  }
  catch (Exception &e) {
    HT_ERRORF("Problem flushing commit log: %s: %s",
              m_smartfd_ptr->to_str().c_str(), e.what());
    error = e.code();
  }

  return error;
}

int CommitLog::sync() {
  lock_guard<mutex> lock(m_mutex);
  int error {};

  try {
    if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
      return Error::CLOSED;
    m_fs->sync(m_smartfd_ptr);
  }
  catch (Exception &e) {
    HT_ERRORF("Problem syncing commit log: %s: %s",
              m_smartfd_ptr->to_str().c_str(), e.what());
    error = e.code();
  }

  return error;
}


int
CommitLog::write(uint64_t cluster_id, DynamicBuffer &buffer, int64_t revision,
                 Filesystem::Flags flags) {
  int error;

  int32_t write_tries = 0;
  try_write_again:
  
  if (m_needs_roll) {
    lock_guard<mutex> lock(m_mutex);
    if ((error = roll()) != Error::OK)
      return error;
  }

  /**
   * Compress and write the commit block
   */
  BlockHeaderCommitLog header(MAGIC_DATA, revision, cluster_id);
  if ((error = compress_and_write(buffer, &header, revision, flags)) != Error::OK){
    if(m_fs->retry_write_ok(m_smartfd_ptr, error, &write_tries, false)){
      m_needs_roll=true;
      goto try_write_again;
    }
    return error;
  }


  /**
   * Roll the log
   */
  if (m_cur_fragment_length > m_max_fragment_size) {
    lock_guard<mutex> lock(m_mutex);
    if ((error = roll()) != Error::OK)
      return error;
  }

  return Error::OK;
}


int CommitLog::link_log(uint64_t cluster_id, CommitLogBase *log_base) {
  lock_guard<mutex> lock(m_mutex);

  string &log_dir = log_base->get_log_dir();

  if (m_linked_log_hashes.count(md5_hash(log_dir.c_str())) > 0) {
    HT_WARNF("Skipping log %s because it is already linked in", log_dir.c_str());
    return Error::OK;
  }

  int64_t link_revision = log_base->get_latest_revision();
  HT_ASSERT(link_revision > 0);
  if (link_revision > m_latest_revision)
    m_latest_revision = link_revision;

  
  int32_t write_tries = 0;
  try_link_log_again:
  
  if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
    m_needs_roll = true;

  int error;
  if (m_needs_roll) {
    if ((error = roll()) != Error::OK)
      return error;
  }
  HT_INFOF("clgc Linking log %s into fragment %d; link_rev=%lld latest_rev=%lld",
           log_dir.c_str(), m_cur_fragment_num, (Lld)link_revision, (Lld)m_latest_revision);


  DynamicBuffer input;
  BlockHeaderCommitLog header(MAGIC_LINK, link_revision, cluster_id);
  input.ensure(header.encoded_length());

  header.set_revision(link_revision);
  header.set_compression_type(BlockCompressionCodec::NONE);
  header.set_data_length(log_dir.length() + 1);
  header.set_data_zlength(log_dir.length() + 1);
  header.set_data_checksum(fletcher32(log_dir.c_str(), log_dir.length()+1));

  header.encode(&input.ptr);
  input.add(log_dir.c_str(), log_dir.length() + 1);

  try {
    size_t amount = input.fill();
    StaticBuffer send_buf(input);

    m_fs->append(m_smartfd_ptr, send_buf);
    m_cur_fragment_length += amount;

    CommitLogFileInfo *file_info = 0;
    if ((error = roll(&file_info)) != Error::OK)
      return error;

    file_info->verify();
    file_info->purge_dirs.insert(log_dir);

    for (auto fi : log_base->fragment_queue()) {
      if (fi->parent == 0) {
        fi->parent = file_info;
        file_info->references++;
      }
      m_fragment_queue.push_back(fi);
    }
    log_base->fragment_queue().clear();

    struct LtClfip swo;
    sort(m_fragment_queue.begin(), m_fragment_queue.end(), swo);

  }
  catch (Hypertable::Exception &e) {
    HT_ERRORF("Problem linking external log into commit log - %s", e.what());
    if(m_fs->retry_write_ok(m_smartfd_ptr, e.code(), &write_tries, false)){
      m_needs_roll=true;
      goto try_link_log_again;
    }
    return e.code();
  }

  m_linked_log_hashes.insert(md5_hash(log_dir.c_str()));

  return Error::OK;
}


int CommitLog::close() {
  lock_guard<mutex> lock(m_mutex);

  try {
    if (m_smartfd_ptr && m_smartfd_ptr->valid()){
      m_fs->close(m_smartfd_ptr);
      m_smartfd_ptr = nullptr;
    }
  }
  catch (Hypertable::Exception &e) {
    HT_ERRORF("Problem closing commit log file '%s' - %s (%s)",
              m_smartfd_ptr->to_str().c_str(), e.what(),
              Error::get_text(e.code()));
    return e.code();
  }

  return Error::OK;
}


int CommitLog::purge(int64_t revision, StringSet &remove_ok_logs,
                     StringSet &removed_logs, string *trace) {
  lock_guard<mutex> lock(m_mutex);

  if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
    return Error::CLOSED;

  if (trace) {
    *trace += format("--- Reap set begin (%s) ---\n", m_log_dir.c_str());
    for (const auto fi : m_reap_set)
      *trace += fi->to_str(remove_ok_logs) + "\n";
    *trace += format("--- Reap set end (%s) ---\n", m_log_dir.c_str());
  }

  // Process "reap" set
  std::set<CommitLogFileInfo *>::iterator rm_iter, iter = m_reap_set.begin();
  while (iter != m_reap_set.end()) {
    if ((*iter)->references == 0 && 
        ((*iter)->remove_ok(remove_ok_logs) || !m_range_reference_required)) {
      remove_file_info(*iter, removed_logs);
      delete *iter;
      rm_iter = iter++;
      m_reap_set.erase(rm_iter);
    }
    else
      ++iter;
  }

  CommitLogFileInfo *fi;
  while (!m_fragment_queue.empty()) {
    fi = m_fragment_queue.front();
    if (fi->revision < revision &&
        (fi->remove_ok(remove_ok_logs) || !m_range_reference_required)) {
      if (fi->references == 0) {
        remove_file_info(fi, removed_logs);
        delete fi;
      }
      else
        m_reap_set.insert(fi);
      m_fragment_queue.pop_front();
    }
    else {
      string msg = format("purge(%s,rev=%llu) breaking on %s",
                          m_log_dir.c_str(), (Llu)revision,
                          fi->to_str(remove_ok_logs).c_str());
      HT_INFOF("%s", msg.c_str());
      if (trace)
        *trace += msg + "\n";
      break;
    }
  }

  return Error::OK;
}


void CommitLog::remove_file_info(CommitLogFileInfo *fi, StringSet &removed_logs) {
  string fname;

  fi->verify();

  // Remove linked log directores
  for (const auto &logdir : fi->purge_dirs) {
    try {
      HT_INFOF("Removing linked log directory '%s' because all fragments have been removed", logdir.c_str());
      removed_logs.insert(logdir);
      m_fs->rmdir(logdir);
    }
    catch (Exception &e) {
      HT_ERRORF("Problem removing log directory '%s' (%s - %s)",
                logdir.c_str(), Error::get_text(e.code()), e.what());
    }
  }

  // Remove fragment file
  try {
    fname = fi->log_dir + "/" + fi->num;
    HT_INFOF("Removing log fragment '%s' revision=%lld, parent=%lld",
             fname.c_str(), (Lld)fi->revision, (Lld)fi->parent);
    m_fs->remove(fname);
  }
  catch (Exception &e) {
    if (e.code() != Error::FSBROKER_BAD_FILENAME &&
        e.code() != Error::FSBROKER_FILE_NOT_FOUND)
      HT_ERRORF("Problem removing log fragment '%s' (%s - %s)",
                fname.c_str(), Error::get_text(e.code()), e.what());
  }

  // Decrement parent reference count
  if (fi->parent) {
    HT_ASSERT(fi->parent->references > 0);
    fi->parent->references--;
  }

}

int CommitLog::roll(CommitLogFileInfo **clfip) {

  if(m_smartfd_ptr->valid()){
    if (m_latest_revision == TIMESTAMP_MIN)
      return Error::OK;
    try {
      m_fs->close(m_smartfd_ptr);
    }
    catch (Exception &e) {
      HT_ERRORF("Problem closing commit log fragment: %s: %s",
		            m_smartfd_ptr->to_str().c_str(), e.what());
      //return e.code();
    }
  }
  m_needs_roll = true;

  CommitLogFileInfo *file_info = new CommitLogFileInfo();
  if (clfip){
    *clfip = 0;
    *clfip = file_info;
  }
  file_info->log_dir = m_log_dir;
  file_info->log_dir_hash = md5_hash(m_log_dir.c_str());
  file_info->num = m_cur_fragment_num;
  file_info->size = m_cur_fragment_length;

  if(m_cur_fragment_length > (int64_t)CommitLogBlockStream::header_size()){
    assert(m_latest_revision != TIMESTAMP_MIN); // only if cur_log not empty
    file_info->revision = m_latest_revision;
  } else 
    file_info->revision = TIMESTAMP_MIN; // a sym-ref log-fragment (skippable)
  m_latest_revision = TIMESTAMP_MIN;

  if (m_fragment_queue.empty()
     || m_fragment_queue.back()->revision < file_info->revision)
    m_fragment_queue.push_back(file_info);
  else {
    m_fragment_queue.push_back(file_info);
    struct LtClfip swo;
    sort(m_fragment_queue.begin(), m_fragment_queue.end(), swo);
  }

  m_cur_fragment_num++;

  int error = create_next_log();
  if(error == Error::OK)
    m_needs_roll = false;

  return error;
}


int
CommitLog::compress_and_write(DynamicBuffer &input, BlockHeader *header,
                              int64_t revision, Filesystem::Flags flags) {
  lock_guard<mutex> lock(m_mutex);

  if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
    return Error::CLOSED;

  int error = Error::OK;
  DynamicBuffer zblock;
  
  bool ownership=input.own;
  input.own=false;

  // Compress block and kick off log write (protected by lock)
  try {
    m_compressor->deflate(input, zblock, *header);

    size_t amount = zblock.fill();
    StaticBuffer send_buf(zblock);

    m_fs->append(m_smartfd_ptr, send_buf, flags);
    assert(revision != 0);
    if (revision > m_latest_revision)
      m_latest_revision = revision;
    m_cur_fragment_length += amount;

  }
  catch (Exception &e) {
    HT_ERRORF("Problem writing commit log: %s: %s",
              m_smartfd_ptr->to_str().c_str(), e.what());
    error = e.code();
  }
  input.own=ownership;

  return error;
}


bool CommitLog::load_cumulative_size_map(CumulativeSizeMap &cumulative_size_map) {
  lock_guard<mutex> lock(m_mutex);

  if (!m_smartfd_ptr || !m_smartfd_ptr->valid()){ //opt, wait  OR roll
    HT_WARNF("Commit log '%s' has been closed, no active commitlog fragment.",
             m_log_dir.c_str());
    return false;
  }

  int64_t cumulative_total = 0;
  uint32_t distance = 0;
  CumulativeFragmentData frag_data;
  
  memset(&frag_data, 0, sizeof(frag_data));

  if (m_latest_revision != TIMESTAMP_MIN) {
    frag_data.size = m_cur_fragment_length;
    frag_data.fragno = m_cur_fragment_num;
    cumulative_size_map[m_latest_revision] = frag_data;
  }

  for (std::deque<CommitLogFileInfo *>::reverse_iterator iter
       = m_fragment_queue.rbegin(); iter != m_fragment_queue.rend(); ++iter) {
    frag_data.size = (*iter)->size;
    frag_data.fragno = (*iter)->num;
    cumulative_size_map[(*iter)->revision] = frag_data;
  }

  for (CumulativeSizeMap::reverse_iterator riter = cumulative_size_map.rbegin();
       riter != cumulative_size_map.rend(); riter++) {
    (*riter).second.distance = distance++;
    cumulative_total += (*riter).second.size;
    (*riter).second.cumulative_size = cumulative_total;
  }

  return true;
}


void CommitLog::get_stats(const string &prefix, string &result) {
  lock_guard<mutex> lock(m_mutex);

  /* current log fragment not in m_fragment_queue for m_smartfd_ptr condition
  if (!m_smartfd_ptr || !m_smartfd_ptr->valid())
    HT_THROWF(Error::CLOSED, "Commit log '%s' has been closed", m_log_dir.c_str());
  */

  try {
    for (const auto frag : m_fragment_queue) {
      result += prefix + String("-log-fragment[") + frag->num + "]\tsize\t" + frag->size + "\n";
      result += prefix + String("-log-fragment[") + frag->num + "]\trevision\t" + frag->revision + "\n";
      result += prefix + String("-log-fragment[") + frag->num + "]\tdir\t" + frag->log_dir + "\n";
    }
    result += prefix + String("-log-fragment[") + m_cur_fragment_num + "]\tsize\t" + m_cur_fragment_length + "\n";
    result += prefix + String("-log-fragment]") + m_cur_fragment_num + "]\trevision\t" + m_latest_revision + "\n";
    result += prefix + String("-log-fragment]") + m_cur_fragment_num + "]\tdir\t" + m_log_dir + "\n";
  }
  catch (Hypertable::Exception &e) {
    HT_ERROR_OUT << "Problem getting stats for log fragments" << HT_END;
    HT_THROW(e.code(), e.what());
  }
}


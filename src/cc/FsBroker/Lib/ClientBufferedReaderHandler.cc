/*
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
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

#include <Common/Compat.h>

#include "Client.h"

#include <AsyncComm/Protocol.h>

#include <Common/Error.h>

using namespace Hypertable;
using namespace Hypertable::FsBroker::Lib;
using namespace std;

ClientBufferedReaderHandler::ClientBufferedReaderHandler(
    Client *client, Filesystem::SmartFdPtr smartfd_ptr, uint32_t buf_size,
    uint32_t outstanding, uint64_t start_offset, uint64_t end_offset) :
    m_client(client), m_smartfd_ptr(smartfd_ptr), 
    m_read_size(buf_size), m_max_outstanding(outstanding), 
    m_actual_offset(start_offset), m_end_offset(end_offset), 
    m_outstanding_offset(start_offset), m_outstanding(0), m_eof(false) {
 
  /**
   * Seek to initial offset
   */
  if (m_outstanding_offset > 0) {
    try { m_client->seek(m_smartfd_ptr, m_outstanding_offset); }
    catch (...) {
      m_eof = true;
      throw;
    }
  }
  
  m_ptr = m_end_ptr = 0;
  {
    lock_guard<mutex> lock(m_mutex);
    read_ahead();
  }
}

ClientBufferedReaderHandler::~ClientBufferedReaderHandler() {
  /*
  // wait and keep the handler method
  // for outstanding events coming in the AsyncComm
  HT_INFOF("~ClientBufferedReaderHandler %s", m_smartfd_ptr->to_str().c_str());
  try {
    unique_lock<mutex> lock(m_mutex);
    do{
      while(!m_queue.empty()){
        m_queue.pop();
        m_outstanding--;
      }
      if(m_outstanding > 0)
        m_cond.wait(lock, [this](){ return !m_queue.empty(); });
      else
        break;
    } while(true);
  catch (...) {
    HT_ERROR("synchronization error");
  }
  HT_INFOF("~ClientBufferedReaderHandler finsih %s ", m_smartfd_ptr->to_str().c_str());
  */
}


void ClientBufferedReaderHandler::handle(EventPtr &event) {
  lock_guard<mutex> lock(m_mutex);
  m_queue.push(event);
  m_cond.notify_all();
  return;
}


uint32_t ClientBufferedReaderHandler::read_response(){
  EventPtr event;
  {
    lock_guard<mutex> lock(m_mutex);
    event = m_queue.front();
  }
  uint32_t amount;
  uint64_t offset;
  int e_code;
	String e_desc;

  if (event->type == Event::MESSAGE 
      && Protocol::response_code(event.get()) == Error::OK){
    try{
      m_client->decode_response_read(
        m_smartfd_ptr, event, (const void **)&m_ptr, &offset, &amount);
      return amount;
    }
	  catch (Exception &e) {
      e_code = e.code();
	    e_desc = format("Error reading %u bytes from FS: %s",
		    (unsigned)m_read_size, m_smartfd_ptr->to_str().c_str());
    }
  }
  else if (event->type == Event::MESSAGE) {
    e_code = Protocol::response_code(event.get());
    e_desc = Protocol::string_format_message(event);
  }
  else if (event->type == Event::ERROR) {
    e_code = event->error;
    e_desc = event->to_str();
  }
  else {
    e_code = Error::FAILED_EXPECTATION;
    e_desc = event->to_str();
  }



  // Synchronize readaheads, expel queue and expected outstanding
  {
    
	  
    HT_WARNF("FsClient sync-fallback for readahead-buffer to Error %d - %s, "
             "at outstanding: %lu in queue: %lu, %s", 
             e_code, e_desc.c_str(), 
             (uint64_t)m_outstanding, m_queue.size(), 
             m_smartfd_ptr->to_str().c_str());
    do{
      {
      unique_lock<mutex> lock(m_mutex);
      while(!m_queue.empty()){
        m_queue.pop();
        m_outstanding--;
      }
      if(m_outstanding > 0)
        m_cond.wait(lock, [this](){ return !m_queue.empty(); });
      else
        break;
      }
    } while(true);
    // add back current event
    m_queue.push(event);
  }
	HT_WARNF("FsClient sync-fallback expel queue and expected outstanding, %s", 
             m_smartfd_ptr->to_str().c_str());


  uint64_t pos = m_smartfd_ptr->pos();
  try_again:

  if(m_client->wait_for_connection(e_code, e_desc)) {
    try{
      m_client->open(m_smartfd_ptr);
		  if(pos > 0)
				m_client->seek(m_smartfd_ptr, pos);

      if(m_actual_offset != pos || pos != m_smartfd_ptr->pos())
        HT_ERRORF("FsClient sync-fallback, actual_offset: %lu pos: %lu fd->pos: %lu , %s", 
          m_actual_offset, pos, m_smartfd_ptr->pos(), m_smartfd_ptr->to_str().c_str());
	    
      DispatchHandlerSynchronizer sync_handler;
		  m_client->read(m_smartfd_ptr, m_read_size, &sync_handler);

	    EventPtr event_sync;
		  if (!sync_handler.wait_for_reply(event_sync))
			  HT_THROW(Protocol::response_code(event_sync.get()), 
				  Protocol::string_format_message(event_sync));
      
      m_client->decode_response_read(
        m_smartfd_ptr, event_sync, (const void **)&m_ptr, &offset, &amount);
      m_outstanding_offset = m_smartfd_ptr->pos();
      m_outstanding = 1;
      return amount;
	  }
	  catch (Exception &e) {
      e_code = e.code();
      e_desc = format("Error readahead-buffer, sync-fallback read "
        "%u bytes from FS: %s", 
        (unsigned)m_read_size, m_smartfd_ptr->to_str().c_str());

		  goto try_again;
    }
  }

  HT_THROW(e_code, e_desc);
  return 0;
}


size_t
ClientBufferedReaderHandler::read(void *buf, size_t len) {

  uint8_t *ptr = (uint8_t *)buf;
  long nleft = len;
  long nread = 0;
  long available, fill;
  uint32_t amount;

  do {
    {
      unique_lock<mutex> lock(m_mutex);
      m_cond.wait(lock, [this](){ return !m_queue.empty() || (m_eof && m_ptr == 0); });
      if (m_ptr == 0 && m_queue.empty())
        HT_THROW(Error::FSBROKER_EOF, "short read (empty queue)");
    }

    read_more:
    if (m_ptr == 0) {
      amount = read_response();
      if (amount < m_read_size)
        m_eof = true;
      m_end_ptr = m_ptr + amount;
      m_actual_offset += amount;
      m_outstanding--;
    }

    available = m_end_ptr - m_ptr;

    if (available > 0) {
      fill = available>nleft ? nleft : available;
      memcpy(ptr, m_ptr, fill);
      nread += fill;
      nleft -= fill;
      m_ptr += fill;
      if(nleft > 0)
        ptr += fill;
    }
    if ((m_end_ptr - m_ptr) == 0) {
      lock_guard<mutex> lock(m_mutex);
      m_queue.pop();
      m_ptr = 0;
    }
    if(!m_eof)
      read_ahead();

    if(nleft == 0)
      break;

    {
      lock_guard<mutex> lock(m_mutex);
      if (m_ptr != 0 || !m_queue.empty())
        goto read_more;
    }
    
  } while(!m_eof || m_outstanding > 0);

  /*
  {
    lock_guard<mutex> lock(m_mutex);
    HT_INFOF("read, return nleft: %ld m_queue: %lu nread: %ld, available: %ld, fill: %ld, m_eof: %d, %s", 
              nleft, m_queue.size(), nread, available, fill, m_eof, m_smartfd_ptr->to_str().c_str());
  }
  */
  return nread;
}



void ClientBufferedReaderHandler::read_ahead() {

  if(m_max_outstanding == m_outstanding)
    return;

  uint32_t n = m_max_outstanding - m_outstanding;
  uint32_t toread;
  
  for (;m_outstanding<n; m_outstanding++) {
      if (m_end_offset && (m_end_offset-m_outstanding_offset) < m_read_size) {
        if ((toread = (uint32_t)(m_end_offset - m_outstanding_offset)) == 0)
          break;
      }
      else
        toread = m_read_size;

      try { m_client->read(m_smartfd_ptr, toread, this); }
      catch (...) {
        m_eof = true;
        throw;
      }
      m_outstanding_offset += toread;
  }
}
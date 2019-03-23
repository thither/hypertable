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
    m_read_size(buf_size), m_eof(false),
    m_error(Error::OK) {
  
  m_max_outstanding = outstanding;
  m_end_offset = end_offset;
  m_outstanding_offset = start_offset;
  m_actual_offset = start_offset;

  /**
   * Seek to initial offset
   */
  if (start_offset > 0) {
    try { m_client->seek(m_smartfd_ptr, start_offset); }
    catch (...) {
      m_eof = true;
      throw;
    }
  }

  {
    lock_guard<mutex> lock(m_mutex);
    uint32_t toread;

    for (m_outstanding=0; m_outstanding<m_max_outstanding; m_outstanding++) {
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
    m_ptr = m_end_ptr = 0;
  }
}



ClientBufferedReaderHandler::~ClientBufferedReaderHandler() {
  try {
    unique_lock<mutex> lock(m_mutex);
    m_eof = true;

    m_cond.wait(lock, [this](){ return m_outstanding == 0; });
  }
  catch (...) {
    HT_ERROR("synchronization error");
  }
}



/**
 *
 */
void ClientBufferedReaderHandler::handle(EventPtr &event) {
  lock_guard<mutex> lock(m_mutex);
  m_outstanding--;

  uint32_t amount;
  uint64_t offset;
  const void *data;

  int e_code;
	String e_desc;

  if (event->type == Event::MESSAGE 
      && Protocol::response_code(event.get()) == Error::OK){
    try{
      m_client->decode_response_read(
        m_smartfd_ptr, event, &data, &offset, &amount);
      m_actual_offset += amount;
      if (amount < m_read_size)
        m_eof = true;
      
      m_queue.push(event);
      m_cond.notify_all();
      return;
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


  int64_t pos = m_smartfd_ptr->pos();
  
	HT_WARNF("FsClient sync-fallback for readahead-buffer to Error %d - %s, %s", 
           e_code, e_desc.c_str(), m_smartfd_ptr->to_str().c_str());
  try_again:
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event_sync;
  if(m_client->wait_for_connection(e_code, e_desc)) {
    try{
      m_client->open(m_smartfd_ptr);
		  if(pos > 0)
				m_client->seek(m_smartfd_ptr, pos);

		  m_client->read(m_smartfd_ptr, m_read_size, &sync_handler);
		  if (!sync_handler.wait_for_reply(event_sync))
			  HT_THROW(Protocol::response_code(event_sync.get()), 
				  Protocol::string_format_message(event_sync));
      
      m_client->decode_response_read(
        m_smartfd_ptr, event_sync, &data, &offset, &amount);
      m_actual_offset += amount;
      if (amount < m_read_size)
        m_eof = true;
      
      m_queue.push(event_sync);
      m_cond.notify_all();
      return;
	  }
	  catch (Exception &e) {
      e_code = e.code();
      e_desc = format("Error readahead-buffer, sync-fallback read "
        "%u bytes from FS: %s", 
        (unsigned)m_read_size, m_smartfd_ptr->to_str().c_str());
		  goto try_again;
    }
  }

  m_error_msg = e_desc;
  HT_ERRORF("%s", m_error_msg.c_str());
  m_eof = true;
  m_cond.notify_all();
}


     

size_t
ClientBufferedReaderHandler::read(void *buf, size_t len) {
  unique_lock<mutex> lock(m_mutex);
  uint8_t *ptr = (uint8_t *)buf;
  long nleft = len;
  long available, nread;

  while (true) {

    m_cond.wait(lock, [this](){ return !m_queue.empty() || m_eof; });

    if (m_error != Error::OK)
      HT_THROW(m_error, m_error_msg);

    if (m_queue.empty())
      HT_THROW(Error::FSBROKER_EOF, "short read (empty queue)");

    if (m_ptr == 0) {
      uint64_t offset;
      uint32_t amount;
      EventPtr &event = m_queue.front();
      m_client->decode_response_read(
        m_smartfd_ptr, event, (const void **)&m_ptr, &offset, &amount);
      m_end_ptr = m_ptr + amount;
    }

    available = m_end_ptr - m_ptr;

    if (available >= nleft) {
      memcpy(ptr, m_ptr, nleft);
      nread = len;
      m_ptr += nleft;
      if ((m_end_ptr - m_ptr) == 0) {
        m_queue.pop();
        m_ptr = 0;
        read_ahead();
      }
      break;
    }
    else if (available == 0) {
      if (m_eof && m_queue.size() == 1) {
        m_queue.pop();
        m_ptr = m_end_ptr = 0;
        nread = len - nleft;
        break;
      }
    }

    memcpy(ptr, m_ptr, available);
    ptr += available;
    nleft -= available;
    m_queue.pop();
    m_ptr = 0;
    read_ahead();
  }
  
  return nread;
}



/**
 *
 */
void ClientBufferedReaderHandler::read_ahead() {
  uint32_t n = m_max_outstanding - (m_outstanding + m_queue.size());
  uint32_t toread;

  HT_ASSERT(m_max_outstanding >= (m_outstanding + m_queue.size()));

  if (m_eof)
    return;

  for (uint32_t i=0; i<n; i++) {
    if (m_end_offset && (m_end_offset-m_outstanding_offset) < m_read_size) {
      if ((toread = (uint32_t)(m_end_offset - m_outstanding_offset)) == 0)
        break;
    }
    else
      toread = m_read_size;

    try { m_client->read(m_smartfd_ptr, toread, this); }
    catch(...) {
      m_eof = true;
      throw;
    }
    m_outstanding++;
    m_outstanding_offset += toread;
  }
}



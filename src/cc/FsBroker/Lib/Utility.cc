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

/// @file
/// Definitions of utility functions.
/// This file contains definitions for utility functions for manipulating files
/// in the brokered filesystem.

#include <Common/Compat.h>

#include "Utility.h"

#include <AsyncComm/Protocol.h>
#include <AsyncComm/DispatchHandlerSynchronizer.h>

#include <Common/Error.h>
#include <Common/Logger.h>

#include <cstdio>

using namespace std;
using namespace Hypertable;

namespace {
  const int BUFFER_SIZE = 32768;
}

void FsBroker::Lib::copy(ClientPtr &client, const std::string &from,
                         const std::string &to, int64_t offset) {
  DispatchHandlerSynchronizer sync_handler;
  Filesystem::SmartFdPtr from_smartfd_ptr = Filesystem::SmartFd::make_ptr(from, 0);
  Filesystem::SmartFdPtr to_smartfd_ptr = Filesystem::SmartFd::make_ptr(
    to, Filesystem::OPEN_FLAG_OVERWRITE);
  
  int32_t write_tries = 0;
  try_write_again:
  try {
    
    client->open(from_smartfd_ptr);
    if (offset > 0)
      client->seek(from_smartfd_ptr, offset);

    client->create(to_smartfd_ptr, -1, -1, -1);    

    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);

    EventPtr event;
    uint8_t *dst;
    uint32_t amount;

    while (sync_handler.wait_for_reply(event)) {

      client->decode_response_pread(
        from_smartfd_ptr, event, (const void **)&dst, (uint64_t *)&offset, &amount);

      StaticBuffer send_buf;
      if (amount > 0) {
        send_buf.set(dst, amount, false);
        client->append(to_smartfd_ptr, send_buf);
      }

      if (amount < (uint32_t)BUFFER_SIZE) {
        sync_handler.wait_for_reply(event);
        break;
      }

      client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    }

    client->close(to_smartfd_ptr);
    client->close(from_smartfd_ptr);

  }
  catch (Exception &e) {
    if(from_smartfd_ptr->valid()
       && client->retry_write_ok(to_smartfd_ptr, e.code(), &write_tries)){
      try{client->close(from_smartfd_ptr);}catch(...){}
      goto try_write_again;
    }
    
    if (from_smartfd_ptr->valid())
      client->close(from_smartfd_ptr);
    if (to_smartfd_ptr->valid())
      client->close(to_smartfd_ptr);

		HT_THROWF(e.code(), "copy from %s to %s", 
      from_smartfd_ptr->to_str().c_str(), to_smartfd_ptr->to_str().c_str());
  }

}

void FsBroker::Lib::copy_from_local(ClientPtr &client, 
  const string &from, const string &to, int64_t offset, int32_t replication) {

  Filesystem::SmartFdPtr to_smartfd_ptr = Filesystem::SmartFd::make_ptr(
    to, Filesystem::OPEN_FLAG_OVERWRITE);

  FsBroker::Lib::copy_from_local(client, from, to_smartfd_ptr, offset, replication);
}

void FsBroker::Lib::copy_from_local(ClientPtr &client, 
  const string &from, Filesystem::SmartFdPtr to_smartfd_ptr, 
  int64_t offset, int32_t replication) {
  FsBroker::Lib::copy_from_local(client.get(), from, to_smartfd_ptr, offset, replication);
}

void FsBroker::Lib::copy_from_local(Client* client, 
  const string &from, Filesystem::SmartFdPtr to_smartfd_ptr, 
  int64_t offset, int32_t replication) {

	HT_INFOF("copying from %s to %s", from.c_str(), to_smartfd_ptr->to_str().c_str());

  DispatchHandlerSynchronizer sync_handler;
  FILE *fp = 0;
  size_t nread;
  uint8_t *buf;
  StaticBuffer send_buf;
  uint32_t outstanding;
  uint32_t max_outstanding = 5;

  int32_t write_tries = 0;
  try_write_again: 

  outstanding = 0;
  try {

    if ((fp = fopen(from.c_str(), "r")) == 0)
      HT_THROW(Error::EXTERNAL, strerror(errno));

    if (offset > 0) {
      if (fseek(fp, (long)offset, SEEK_SET) != 0)
        HT_THROW(Error::EXTERNAL, strerror(errno));
    }

    client->create(to_smartfd_ptr, -1, replication, -1);

    while (max_outstanding>0) {

      buf = new uint8_t [BUFFER_SIZE];
      if ((nread = fread(buf, 1, BUFFER_SIZE, fp)) == 0){
        max_outstanding = 0;
      } else {
        send_buf.set(buf, nread, true);
        client->append(to_smartfd_ptr, send_buf, Filesystem::Flags::NONE, &sync_handler);
        outstanding++;
      }

      while (outstanding > max_outstanding) {
        EventPtr event_ptr;
        if (!sync_handler.wait_for_reply(event_ptr))
         HT_THROWF(Protocol::response_code(event_ptr),
                  "Problem appending, %s : %s",
                    to_smartfd_ptr->to_str().c_str(),
                    Protocol::string_format_message(event_ptr).c_str());
        outstanding--;
      }
    }
    
    client->close(to_smartfd_ptr);

  }
  catch (Exception &e) {
    if (fp)
      fclose(fp);
    if(client->retry_write_ok(to_smartfd_ptr, e.code(), &write_tries))
      goto try_write_again;

		HT_THROWF(e.code(), "copy from %s to %s", 
      from.c_str(), to_smartfd_ptr->to_str().c_str());
  }
}


void FsBroker::Lib::copy_to_local(ClientPtr &client, 
  const string &from, const string &to, int64_t offset) {
  DispatchHandlerSynchronizer sync_handler;
  FILE *fp = 0;

  Filesystem::SmartFdPtr from_smartfd_ptr = Filesystem::SmartFd::make_ptr(from, 0);
  
  try {

    if ((fp = fopen(to.c_str(), "w+")) == 0)
      HT_THROW(Error::EXTERNAL, strerror(errno));


    client->open(from_smartfd_ptr);

    if (offset > 0)
      client->seek(from_smartfd_ptr, offset);

    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);

    EventPtr event;
    const uint8_t *dst;
    uint32_t amount;

    while (sync_handler.wait_for_reply(event)) {

      client->decode_response_pread(from_smartfd_ptr, 
            event, (const void **)&dst, (uint64_t *)&offset, &amount);

      if (amount > 0) {
        if (fwrite(dst, amount, 1, fp) != 1)
          HT_THROW(Error::EXTERNAL, strerror(errno));
      }

      if (amount < (uint32_t)BUFFER_SIZE) {
        sync_handler.wait_for_reply(event);
        break;
      }

      client->read(from_smartfd_ptr, BUFFER_SIZE, &sync_handler);
    }

    client->close(from_smartfd_ptr);

    fclose(fp);

  }
  catch (Exception &e) {
    if (fp)
      fclose(fp);

		HT_THROWF(e.code(), "copy from %s to %s", 
      from_smartfd_ptr->to_str().c_str(), to.c_str());
  }
  
}


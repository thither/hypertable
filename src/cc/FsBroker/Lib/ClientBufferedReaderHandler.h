/* -*- c++ -*-
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
#ifndef FsBroker_Lib_ClientBufferedReaderHandler_h
#define FsBroker_Lib_ClientBufferedReaderHandler_h

#include <AsyncComm/DispatchHandler.h>

#include <Common/String.h>
#include <Common/Filesystem.h>

#include <condition_variable>
#include <mutex>
#include <queue>


namespace Hypertable {
  

namespace FsBroker {
namespace Lib {

  class Client;

  /// @addtogroup FsBrokerLib
  /// @{

  class ClientBufferedReaderHandler : public DispatchHandler {

  public:
    ClientBufferedReaderHandler(Client *client, 
        Filesystem::SmartFdPtr smartfd_ptr,
        uint32_t buf_size, uint32_t outstanding, uint64_t start_offset,
        uint64_t end_offset);

    virtual ~ClientBufferedReaderHandler();

    virtual void handle(EventPtr &event);

    size_t read(void *buf, size_t len);

  private:

    void read_ahead();
    uint32_t read_response();

    std::mutex              m_mutex;
    std::condition_variable m_cond;
    std::queue<EventPtr>    m_queue;

    Client                  *m_client;
    Filesystem::SmartFdPtr  m_smartfd_ptr;

    uint32_t   m_read_size;
    uint32_t   m_max_outstanding;
    uint64_t   m_actual_offset;
    uint64_t   m_end_offset;
    uint64_t   m_outstanding_offset;
    uint32_t   m_outstanding;
    bool       m_eof;
    
    const uint8_t       *m_ptr;
    const uint8_t       *m_end_ptr;
  };

  typedef std::shared_ptr<ClientBufferedReaderHandler> ClientBufferedReaderHandlerPtr;
  /// @}

}}}

#endif // FsBroker_Lib_ClientBufferedReaderHandler_h


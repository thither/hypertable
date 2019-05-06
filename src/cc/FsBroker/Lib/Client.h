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

/// @file
/// Declarations for Client.
/// This file contains declarations for Client, a client proxy class for the
/// file system broker.

#ifndef FsBroker_Lib_Client_h
#define FsBroker_Lib_Client_h

#include <AsyncComm/Comm.h>
#include <AsyncComm/ConnectionManager.h>
#include <AsyncComm/DispatchHandlerSynchronizer.h>

#include <Common/Filesystem.h>
#include <Common/InetAddr.h>
#include <Common/Properties.h>
#include <Common/Status.h>

#include <FsBroker/Lib/ClientBufferedReaderHandler.h>

#include <memory>
#include <mutex>
#include <unordered_map>

namespace Hypertable {
  namespace FsBroker {
    namespace Lib {
      
      // Client's SmartFd
      struct ClientFd : Filesystem::SmartFd {
        public:
        ClientFd(const String &name, uint32_t flag_s)
                : Filesystem::SmartFd(name, flag_s){}

        ClientFd(const String &name, uint32_t flag_s, int32_t fileno, 
                uint64_t position)
                : Filesystem::SmartFd(name, flag_s, fileno, position){}

        virtual ~ClientFd(){}

        ClientBufferedReaderHandlerPtr reader = nullptr;
      };
      typedef std::shared_ptr<ClientFd> ClientFdPtr;
      
      /// @addtogroup FsBrokerLib
      /// @{
        
      /** File Descriptor Object used with the FsBroker-Client
      */


      /** Proxy class for FS broker.  As specified in the general contract for a
      * Filesystem, commands that operate on the same file descriptor are
      * serialized by the underlying filesystem.  In other words, if you issue
      * three asynchronous commands, they will get carried out and their
      * responses will come back in the same order in which they were issued.
      */
      class Client : public Filesystem {
      public:

        virtual ~Client();

        /** Constructor with explicit values.  Connects to the FS broker at the
        * address given by the addr argument and uses the timeout argument for
        * the request timeout values.
        *
        * @param conn_manager_ptr smart pointer to connection manager
        * @param addr address of FS broker to connect to
        * @param timeout_ms timeout value in milliseconds to use in requests
        */
        Client(ConnectionManagerPtr &conn_manager_ptr,
          const sockaddr_in &addr, uint32_t timeout_ms);

        /** Constructor with config var map.  The following properties are read
        * to determine the location of the broker and the request timeout value:
        * <pre>
        * FsBroker.port
        * FsBroker.host
        * FsBroker.timeout
        * </pre>
        *
        * @param conn_manager_ptr smart pointer to connection manager
        * @param cfg config variables map
        */
        Client(ConnectionManagerPtr &conn_manager_ptr, PropertiesPtr &cfg);

        /** Constructor without connection manager.
        *
        * @param comm pointer to the Comm object
        * @param addr remote address of already connected FsBroker
        * @param timeout_ms timeout value in milliseconds to use in requests
        */
        Client(Comm *comm, const sockaddr_in &addr, uint32_t timeout_ms);

        /** Convenient contructor for dfs testing
        *
        * @param host - dfs hostname
        * @param port - dfs port
        * @param timeout_ms - timeout (in milliseconds) for requests
        */
        Client(const String &host, int port, uint32_t timeout_ms);

        /** Waits up to max_wait_secs for a connection to be established with the
        * FS broker.
        *
        * @param max_wait_ms maximum amount of time to wait @return true if
        * connected, false otherwise
        */
        bool wait_for_connection(uint32_t max_wait_ms) {
          if (m_conn_mgr)
            return m_conn_mgr->wait_for_connection(m_addr, max_wait_ms);
          return true;
        }
        
        /** Tries to re-connect to FS Broker max retries time estimated to m_timeout_ms X hypretable.fsbroker.ClientRetries 
        return True if re-connected, 
        @param e_code last exception error code
        @param e last exception
        @param e_desc last error description (added to waiting for connection Exception)
        */
        bool wait_for_connection(int e_code, const String &e_desc);

        enum {
          /// Perform immediate shutdown
          SHUTDOWN_FLAG_IMMEDIATE = 0x01
        };
        // CLIENT-NON-FILE-COMMAND
        /** Shuts down the FS broker.  Issues a shutdown command to the FS
        * broker.  If the flag is set to SHUTDOWN_FLAG_IMMEDIATE, then
        * the broker will call exit(0) directly from the I/O reactor thread.
        * Otherwise, a shutdown command will get added to the broker's
        * application queue, allowing the shutdown to be handled more
        * gracefully.
        *
        * @param flags controls how broker gets shut down
        * @param handler response handler
        */
        void shutdown(uint16_t flags, DispatchHandler *handler);
        
        void status(Status &status, Timer *timer = 0) override;
        void decode_response_status(EventPtr &event, Status &status) override;

        void debug(int32_t command, 
          StaticBuffer &serialized_parameters) override;
        void debug(int32_t command, StaticBuffer &serialized_parameters,
          DispatchHandler *handler) override;



        // Methods based on ClientFd

        void open(Filesystem::SmartFdPtr fd_obj, 
          DispatchHandler *handler) override;
        void open(Filesystem::SmartFdPtr fd_obj) override;
        void open_buffered(Filesystem::SmartFdPtr &fd_obj, 
          uint32_t buf_size, uint32_t outstanding,
          uint64_t start_offset = 0, uint64_t end_offset = 0) override;
        void decode_response_open(Filesystem::SmartFdPtr fd_obj, 
          EventPtr &event) override;
        
        void create(Filesystem::SmartFdPtr fd_obj, int32_t bufsz,
          int32_t replication, int64_t blksz, DispatchHandler *handler) override;
        void create(Filesystem::SmartFdPtr fd_obj, int32_t bufsz, 
          int32_t replication, int64_t blksz) override;
        void decode_response_create(Filesystem::SmartFdPtr fd_obj, 
          EventPtr &event) override;

        void close(Filesystem::SmartFdPtr fd_obj, 
          DispatchHandler *handler) override;
        void close(Filesystem::SmartFdPtr fd_obj) override;

        void read(Filesystem::SmartFdPtr fd_obj, size_t amount, 
          DispatchHandler *handler) override;
        size_t read(Filesystem::SmartFdPtr fd_obj, void *dst, 
          size_t amount) override;
        void decode_response_read(Filesystem::SmartFdPtr fd_obj, EventPtr &event, 
          const void **buffer, uint64_t *offset, uint32_t *length) override;

        void pread(Filesystem::SmartFdPtr fd_obj, size_t len, uint64_t offset,
                    bool verify_checksum, DispatchHandler *handler) override;
        size_t pread(Filesystem::SmartFdPtr fd_obj, void *dst, size_t len, 
          uint64_t offset, bool verify_checksum) override;
        void decode_response_pread(Filesystem::SmartFdPtr fd_obj, EventPtr &event, 
          const void **buffer, uint64_t *offset, uint32_t *length) override; 

        void append(Filesystem::SmartFdPtr fd_obj, StaticBuffer &buffer, 
          Flags flags, DispatchHandler *handler) override;
        size_t append(Filesystem::SmartFdPtr fd_obj, StaticBuffer &buffer,
          Flags flags = Flags::NONE) override;
        void decode_response_append(Filesystem::SmartFdPtr fd_obj, 
          EventPtr &event, uint64_t *offset, uint32_t *length) override;

        void seek(Filesystem::SmartFdPtr fd_obj, uint64_t offset,
          DispatchHandler *handler) override;
        void seek(Filesystem::SmartFdPtr fd_obj, uint64_t offset) override;

        void flush(Filesystem::SmartFdPtr fd_obj, 
          DispatchHandler *handler) override;
        void flush(Filesystem::SmartFdPtr fd_obj) override;

        void sync(Filesystem::SmartFdPtr fd_obj) override;

        Filesystem::SmartFdPtr create_local_temp(const String &for_filename) override;
        void append_to_temp(Filesystem::SmartFdPtr smartfd_ptr, StaticBuffer &buffer) override;
        void commit_temp(Filesystem::SmartFdPtr &smartfd_ptr, Filesystem::SmartFdPtr to_smartfd_ptr, int32_t replication) override;


        // Methods based on int-fd, Depreciated FsBroker-Client methods 
        // -- for compatabillity to Common:FileSystem

        void open(const String &name, uint32_t flags, 
          DispatchHandler *handler) override;
        int open(const String &name, uint32_t flags) override;
        int open_buffered(const String &name, uint32_t flags, 
          uint32_t buf_size, uint32_t outstanding, uint64_t start_offset = 0, 
          uint64_t end_offset = 0) override;
        void decode_response_open(EventPtr &event, int32_t *fd) override;

        void create(const String &name, uint32_t flags,
          int32_t bufsz, int32_t replication,
          int64_t blksz, DispatchHandler *handler) override;
        int create(const String &name, uint32_t flags, int32_t bufsz,
          int32_t replication, int64_t blksz) override;
        void decode_response_create(EventPtr &event, int32_t *fd) override;

        void read(int32_t fd, size_t amount, DispatchHandler *handler) override;
        size_t read(int32_t fd, void *dst, size_t amount) override;
        void decode_response_read(EventPtr &event, const void **buffer,
          uint64_t *offset, uint32_t *length) override;

        void pread(int32_t fd, size_t len, uint64_t offset,
                  bool verify_checksum, DispatchHandler *handler) override;
        size_t pread(int32_t fd, void *dst, size_t len, uint64_t offset,
                    bool verify_checksum) override;
        void decode_response_pread(EventPtr &event, const void **buffer,
                                  uint64_t *offset, uint32_t *length) override;

        void append(int32_t fd, StaticBuffer &buffer, Flags flags, 
          DispatchHandler *handler) override;
        size_t append(int32_t fd, StaticBuffer &buffer, 
          Flags flags = Flags::NONE) override;
        void decode_response_append(EventPtr &event,
          uint64_t *offset, uint32_t *length) override;

        void seek(int32_t fd, uint64_t offset, 
          DispatchHandler *handler) override;
        void seek(int32_t fd, uint64_t offset) override;

        void flush(int32_t fd, DispatchHandler *handler) override;
        void flush(int32_t fd) override;

        void sync(int32_t fd) override;

        void close(int32_t fd, DispatchHandler *handler) override;
        void close(int32_t fd) override;



        // Methods based on File/Path name

        void remove(const String &name, DispatchHandler *handler) override;
        void remove(const String &name, bool force = true) override;

        void length(const String &name, bool accurate, 
          DispatchHandler *handler) override;
        int64_t length(const String &name, bool accurate = true) override;
        int64_t decode_response_length(EventPtr &event) override;

        void mkdirs(const String &name, DispatchHandler *handler) override;
        void mkdirs(const String &name) override;

        void rmdir(const String &name, DispatchHandler *handler) override;
        void rmdir(const String &name, bool force = true) override;

        void readdir(const String &name, DispatchHandler *handler) override;
        void readdir(const String &name, std::vector<Dirent> &listing) override;
        void decode_response_readdir(EventPtr &event,
          std::vector<Dirent> &listing) override;

        void exists(const String &name, DispatchHandler *handler) override;
        bool exists(const String &name) override;
        bool decode_response_exists(EventPtr &event) override;

        void rename(const String &src, const String &dst,
          DispatchHandler *handler) override;
        void rename(const String &src, const String &dst) override;


        /** Gets the configured request timeout value.
        *
        * @return timeout value in milliseconds
        */
        uint32_t get_timeout() { return m_timeout_ms; }

        /** Gets the configured write retry limit.
        *
        * @return limit count
        */
        int32_t get_retry_write_limit() override;
        
        /** Determines if it is OK to retry write.
        * Closes a fd if open, removes the previous file
        * if retry limit reached return False 
        * else increment tries_count and return True
        *
        * @param smartfd_ptr the SmartFdPtr 
        * @param e_code Exception code
        * @param tries_count pointer to write_count
        * @return true if OK, otherwise false
        */
        bool retry_write_ok(Filesystem::SmartFdPtr smartfd_ptr, 
														int32_t e_code, int32_t *tries_count,
                            bool del_old=true) override;

      private:

        /// Sends a message to the FS broker.
        /// @param cbuf message to send
        /// @param handler response handler
        /// @param timer Deadline timer
        void send_message(CommBufPtr &cbuf, DispatchHandler *handler, 
          Timer *timer = 0);

        std::mutex m_mutex;
        Comm *m_comm;
        ConnectionManagerPtr m_conn_mgr;
        InetAddr m_addr;
        uint32_t m_timeout_ms;
        gInt32tPtr m_write_retry_limit {};
        gInt32tPtr m_retry_limit {};
        String m_temp_path;
        std::unordered_map<uint32_t, Filesystem::SmartFdPtr> m_buffered_reader_map;
        
        int32_t m_conn_retries = 0;
        bool m_conn_active = true;

      };

      /// Smart pointer to Client
      typedef std::shared_ptr<Client> ClientPtr;

      /// @}

    }
  }
}


#endif // FsBroker_Lib_Client_h

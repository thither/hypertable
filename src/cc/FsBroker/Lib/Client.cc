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
/// Definitions for Client.
/// This file contains definitions for Client, a client proxy class for the
/// file system broker.

#include <Common/Compat.h>

#include "Client.h"

#include "Request/Handler/Factory.h"
#include "Request/Parameters/Append.h"
#include "Request/Parameters/Close.h"
#include "Request/Parameters/Create.h"
#include "Request/Parameters/Debug.h"
#include "Request/Parameters/Exists.h"
#include "Request/Parameters/Flush.h"
#include "Request/Parameters/Length.h"
#include "Request/Parameters/Mkdirs.h"
#include "Request/Parameters/Open.h"
#include "Request/Parameters/Pread.h"
#include "Request/Parameters/Readdir.h"
#include "Request/Parameters/Read.h"
#include "Request/Parameters/Remove.h"
#include "Request/Parameters/Rename.h"
#include "Request/Parameters/Rmdir.h"
#include "Request/Parameters/Seek.h"
#include "Request/Parameters/Shutdown.h"
#include "Request/Parameters/Sync.h"
#include "Response/Parameters/Append.h"
#include "Response/Parameters/Exists.h"
#include "Response/Parameters/Length.h"
#include "Response/Parameters/Open.h"
#include "Response/Parameters/Read.h"
#include "Response/Parameters/Readdir.h"
#include "Response/Parameters/Status.h"

#include <AsyncComm/Comm.h>
#include <AsyncComm/CommBuf.h>
#include <AsyncComm/CommHeader.h>
#include <AsyncComm/Protocol.h>

#include <Common/Config.h>
#include <Common/Error.h>
#include <Common/Filesystem.h>
#include <Common/Logger.h>
#include <Common/Serialization.h>

using namespace Hypertable;
using namespace Serialization;
using namespace Hypertable::FsBroker;
using namespace Hypertable::FsBroker::Lib;
using namespace std;



// Client Initializers
Client::Client(ConnectionManagerPtr &conn_mgr, const sockaddr_in &addr,
	uint32_t timeout_ms)
	: m_conn_mgr(conn_mgr), m_addr(addr), m_timeout_ms(timeout_ms) {
	m_comm = conn_mgr->get_comm();
	conn_mgr->add(m_addr, m_timeout_ms, "FS Broker");
	
  m_write_retry_limit = Config::get_ptr<gInt32t>("FsBroker.WriteRetryLimit");
  m_retry_limit = Config::get_ptr<gInt32t>("FsBroker.RetryLimit");
}

Client::Client(ConnectionManagerPtr &conn_mgr, PropertiesPtr &props)
	: m_conn_mgr(conn_mgr) {
	m_comm = conn_mgr->get_comm();

	String host = props->get_str("FsBroker.Host");
	uint16_t port = props->get_i16("FsBroker.Port");

  m_timeout_ms = props->get_pref<int32_t>(
			{"FsBroker.Timeout", "Hypertable.Request.Timeout"});
  m_write_retry_limit = props->get_ptr<gInt32t>("FsBroker.WriteRetryLimit");
  m_retry_limit =  props->get_ptr<gInt32t>("FsBroker.RetryLimit");
	
	InetAddr::initialize(&m_addr, host.c_str(), port);

	conn_mgr->add(m_addr, m_timeout_ms, "FS Broker");
	
}

Client::Client(Comm *comm, const sockaddr_in &addr, uint32_t timeout_ms)
	: m_comm(comm), m_conn_mgr(0), m_addr(addr), m_timeout_ms(timeout_ms) {
  m_write_retry_limit = Config::get_ptr<gInt32t>("FsBroker.WriteRetryLimit");
  m_retry_limit = Config::get_ptr<gInt32t>("FsBroker.RetryLimit");
}

Client::Client(const String &host, int port, uint32_t timeout_ms)
	: m_timeout_ms(timeout_ms) {
	InetAddr::initialize(&m_addr, host.c_str(), port);
	m_comm = Comm::instance();
	m_conn_mgr = make_shared<ConnectionManager>(m_comm);
	m_conn_mgr->add(m_addr, timeout_ms, "FS Broker");
	if (!m_conn_mgr->wait_for_connection(m_addr, timeout_ms))
		HT_THROW(Error::REQUEST_TIMEOUT,
			"Timed out waiting for connection to FS Broker");
	
  m_write_retry_limit = Config::get_ptr<gInt32t>("FsBroker.WriteRetryLimit");
  m_retry_limit = Config::get_ptr<gInt32t>("FsBroker.RetryLimit");
}

Client::~Client() {
	/** this causes deadlock in RangeServer shutdown
	if (m_conn_mgr)
	m_conn_mgr->remove(m_addr);
	*/
}


// CLIENT-NON-FILE-COMMAND
void
Client::debug(int32_t command, StaticBuffer &serialized_parameters,
	            DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_DEBUG);
	Request::Parameters::Debug params(command);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length(),
		serialized_parameters));
	params.encode(cbuf->get_data_ptr_address());

	try { 
    send_message(cbuf, handler); 
  }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, 
			"Error sending debug command %d request", command);
	}
}

void
Client::debug(int32_t command, StaticBuffer &serialized_parameters) {
	DispatchHandlerSynchronizer sync_handler;

	try {
		debug(command, serialized_parameters, &sync_handler);

	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, 
			"Error sending debug command %d request", command);
	}
}

void
Client::shutdown(uint16_t flags, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_SHUTDOWN);
	Request::Parameters::Shutdown params(flags);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "sending FS shutdown (flags=%d)", (int)flags);
	}
}

void Client::status(Status &status, Timer *timer) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_STATUS);
	CommBufPtr cbuf(new CommBuf(header));

	try {
		send_message(cbuf, &sync_handler, timer);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		decode_response_status(event, status);
	}
	catch (Exception &e) {
		status.set(Status::Code::CRITICAL,
			format("%s - %s", Error::get_text(e.code()), e.what()));
	}
}

void Client::decode_response_status(EventPtr &event, Status &status) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Status params;
	params.decode(&ptr, &remain);
	status = params.status();
}

//local namespace
namespace{
bool is_error_handlable(int e_code){
	return (e_code == Error::COMM_NOT_CONNECTED ||
		 e_code == Error::COMM_BROKEN_CONNECTION ||
		 e_code == Error::COMM_CONNECT_ERROR ||
		 e_code == Error::COMM_SEND_ERROR ||
		 e_code == Error::FSBROKER_BAD_FILE_HANDLE ||
		 e_code == Error::FSBROKER_IO_ERROR ||
		 e_code == Error::CLOSED);
}
}

bool Client::wait_for_connection(int e_code, const String &e_desc) {

	if(!is_error_handlable(e_code)) {
		HT_INFOF("FsClient skip from waiting to error: %d, %s ",  
							e_code, e_desc.c_str());
		return false;
	}
	HT_INFOF("FsClient::wait_for_connection, %d: %s", e_code ,e_desc.c_str());

	lock_guard<mutex> lock(m_mutex);

	if (m_conn_retries == m_retry_limit->get())
		HT_THROW(e_code,
			format("Timed out waiting for connection to FS Broker, tried %d times - %s", 
							m_conn_retries, e_desc.c_str()));

	if(e_code == Error::COMM_NOT_CONNECTED || 
		 e_code == Error::COMM_BROKEN_CONNECTION){
		bool is_active = m_conn_mgr->is_connection_state(
			m_addr, ConnectionManager::State::READY);
		/*
		if(!is_active && !m_conn_active){
			m_conn_mgr->remove(m_addr);
			m_conn_mgr->add(m_addr, m_timeout_ms, "FS Broker");
			m_conn_active = true;
		}
		*/
		if (!is_active && !m_conn_mgr->wait_for_connection(m_addr, m_timeout_ms)){
			m_conn_active = false;
			m_conn_retries++;
			HT_INFOF("FsClient conn-retry: %d failed, to error: %s",  
							m_conn_retries, e_desc.c_str());
		} else {
			HT_INFOF("FsClient conn-established after %d tries to error: %s", 
						 		m_conn_retries, e_desc.c_str());
			m_conn_retries = 0;
			m_conn_active = true;
		}
	}
	return true;
}


void
Client::send_message(CommBufPtr &cbuf, DispatchHandler *handler, Timer *timer) {
	uint32_t deadline = timer ? timer->remaining() : m_timeout_ms;
	int error = m_comm->send_request(m_addr, deadline, cbuf, handler);

	if (error != Error::OK)
		HT_THROWF(error, "FS send_request to %s failed", m_addr.format().c_str());
}


int32_t Client::get_retry_write_limit() { 
  if(m_write_retry_limit == nullptr)
      m_write_retry_limit = Config::get_ptr<gInt32t>("FsBroker.WriteRetryLimit");
  return m_write_retry_limit->get();
}


// Methods based on SmartFdPtr(ClientFdPtr)

ClientFdPtr get_clientfd_ptr(Filesystem::SmartFdPtr smartfd_ptr){
	return std::dynamic_pointer_cast<ClientFd>(smartfd_ptr);
}


bool Client::retry_write_ok(Filesystem::SmartFdPtr smartfd_ptr, 
														int32_t e_code, int32_t *tries_count){
  if(smartfd_ptr->valid()) { 
    try{close(smartfd_ptr);}
    catch(...){}
	}
	if(!smartfd_ptr->filepath().empty()) {
    try{remove(smartfd_ptr->filepath());}
    catch(...){}
  }
	
  if(*tries_count < m_write_retry_limit->get()
     && is_error_handlable(e_code)){
    *tries_count = *tries_count+1; 
		HT_INFOF("FsClient retry_write_ok, try %d to error: %d %s", 
						 *tries_count, e_code, smartfd_ptr->to_str().c_str());
		return true;
  }
	HT_WARNF("FsClient retry_write_ok, FALSE to error: %d %s", 
						 e_code, smartfd_ptr->to_str().c_str());
	return false;
}

// OPEN
void
Client::open(Filesystem::SmartFdPtr smartfd_ptr, DispatchHandler *handler) {
 	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_OPEN);
	Request::Parameters::Open params(smartfd_ptr->filepath(), smartfd_ptr->flags(), 0);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		if(smartfd_ptr->valid())
			try{close(smartfd_ptr);}catch(Exception &e) {}
		send_message(cbuf, handler);
	}
	catch (Exception &e) {
		String e_desc = format("Error sending open request FS: %s", 
			smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
		HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::open(Filesystem::SmartFdPtr smartfd_ptr) {
 	try_again:

	DispatchHandlerSynchronizer sync_handler;
	open(smartfd_ptr, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
	
		decode_response_open(smartfd_ptr, event);
	}
	catch (Exception &e) {
		String e_desc = format("Error opening FS: %s", 
			smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
		HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::open_buffered(Filesystem::SmartFdPtr &smartfd_ptr, uint32_t buf_size,
                      uint32_t outstanding, uint64_t start_offset,
                      uint64_t end_offset) {
	try {
		HT_ASSERT((smartfd_ptr->flags() & Filesystem::OPEN_FLAG_DIRECTIO) == 0 ||
			(HT_IO_ALIGNED(buf_size) &&
				HT_IO_ALIGNED(start_offset) &&
				HT_IO_ALIGNED(end_offset)));
		
		if(!(smartfd_ptr->flags() & OPEN_FLAG_VERIFY_CHECKSUM))
			smartfd_ptr->flags(smartfd_ptr->flags() | OPEN_FLAG_VERIFY_CHECKSUM);
    
		// a clientfd needed for BuffReader
		
   	ClientFdPtr	clientfd_ptr = get_clientfd_ptr(smartfd_ptr);
		if(!clientfd_ptr){
		 	clientfd_ptr = std::make_shared<ClientFd>(
				 smartfd_ptr->filepath(), smartfd_ptr->flags());
			Filesystem::SmartFdPtr new_ptr = 
				std::static_pointer_cast<Filesystem::SmartFd>(clientfd_ptr);
			smartfd_ptr = new_ptr;
		}

		open(smartfd_ptr);
    clientfd_ptr->reader = std::make_shared<ClientBufferedReaderHandler>(
        this, smartfd_ptr, buf_size, outstanding, start_offset, end_offset);
	}
	catch (Exception &e)  {
		HT_THROW2(e.code(), e, format("Error opening buffered FS: %s, buf_size=%u "
													 "outstanding=%u start_offset=%llu end_offset=%llu", 
			smartfd_ptr->to_str().c_str(), buf_size, outstanding, 
			(Llu)start_offset, (Llu)end_offset));
  }
}

void Client::decode_response_open(Filesystem::SmartFdPtr smartfd_ptr, 
																	EventPtr &event) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Open params;
	params.decode(&ptr, &remain);
  
	smartfd_ptr->fd(params.get_fd());
}


// CREATE
void
Client::create(Filesystem::SmartFdPtr smartfd_ptr, int32_t bufsz,
              int32_t replication, int64_t blksz,
              DispatchHandler *handler) {
 	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_CREATE);
	Request::Parameters::Create params(smartfd_ptr->filepath(), 
																		 smartfd_ptr->flags(), 
                                     bufsz, replication, blksz);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		String e_desc = format("Error sending create request FS: %s:", 
			smartfd_ptr->to_str().c_str());
		if (wait_for_connection(e.code(), e_desc))
			goto try_again;
		HT_THROW2(e.code(), e, e_desc.c_str());
	}
}

void
Client::create(Filesystem::SmartFdPtr smartfd_ptr, int32_t bufsz,
               int32_t replication, int64_t blksz) {
 	try_again:

	DispatchHandlerSynchronizer sync_handler;
	create(smartfd_ptr, bufsz, replication, blksz, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
		decode_response_create(smartfd_ptr, event);
	}
	catch (Exception &e) {
		String e_desc = format("Error creating FS: %s:", 
			smartfd_ptr->to_str().c_str());
		if (wait_for_connection(e.code(), e_desc))
			goto try_again;
		HT_THROW2(e.code(), e, e_desc.c_str());
	}
}

void Client::decode_response_create(Filesystem::SmartFdPtr smartfd_ptr, 
																		EventPtr &event) {
	decode_response_open(smartfd_ptr, event);
}


// CLOSE
void
Client::close(Filesystem::SmartFdPtr smartfd_ptr, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_CLOSE);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Close params(smartfd_ptr->fd());
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	// clear irrelevant data, keeping filepath & flags
	smartfd_ptr->fd(-1);
	smartfd_ptr->pos(0);
	
  ClientFdPtr clientfd_ptr = get_clientfd_ptr(smartfd_ptr);
	if (clientfd_ptr && clientfd_ptr->reader)
		clientfd_ptr->reader = nullptr;

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e,  "Error closing FS: %s",
			smartfd_ptr->to_str().c_str());
	}
}

void
Client::close(Filesystem::SmartFdPtr smartfd_ptr) {

	DispatchHandlerSynchronizer sync_handler;
	close(smartfd_ptr, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e,  "Error closing FS: %s", 
			smartfd_ptr->to_str().c_str());
	}
}


// READ
void
Client::read(Filesystem::SmartFdPtr smartfd_ptr, size_t len, 
						 DispatchHandler *handler) {
	uint64_t pos = smartfd_ptr->pos();
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_READ);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Read params(smartfd_ptr->fd(), len);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending read request %u bytes from FS: %s",
		 (unsigned)len, smartfd_ptr->to_str().c_str());
	  if(wait_for_connection(e.code(), e_desc)) {
		  open(smartfd_ptr);
			if(pos > 0)
				seek(smartfd_ptr, pos);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}

size_t
Client::read(Filesystem::SmartFdPtr smartfd_ptr, void *dst, size_t len) {
	uint64_t pos = smartfd_ptr->pos();
	try_again:
	
  ClientFdPtr clientfd_ptr = get_clientfd_ptr(smartfd_ptr);
	if (clientfd_ptr && clientfd_ptr->reader)
		return clientfd_ptr->reader->read(dst, len);

	DispatchHandlerSynchronizer sync_handler;
	read(smartfd_ptr, len, &sync_handler);

  try{
		EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
		
		uint32_t length;
		uint64_t offset;
		const void *data;
		decode_response_read(smartfd_ptr, event, &data, &offset, &length);
		HT_ASSERT(length <= len);
		memcpy(dst, data, length);
		return length;
	}
	catch (Exception &e) {
	  String e_desc = format("Error reading %u bytes from FS: %s",
		 (unsigned)len, smartfd_ptr->to_str().c_str());
	  if(wait_for_connection(e.code(), e_desc)) {
			// return pread(smartfd_ptr, dst, len, smartfd_ptr->pos(), false);
		  open(smartfd_ptr);
			if(pos > 0)
				seek(smartfd_ptr, pos);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void Client::decode_response_read(Filesystem::SmartFdPtr smartfd_ptr, 
	EventPtr &event, const void **buffer,	uint64_t *offset, uint32_t *length) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Read params;
	params.decode(&ptr, &remain);
	*offset = params.get_offset();
	*length = params.get_amount();

	if (*length == (uint32_t)-1) {
		*length = 0;
		return;
	}

	if (remain < (size_t)*length)
		HT_THROWF(Error::RESPONSE_TRUNCATED, "%lu < %lu", (Lu)remain, (Lu)*length);

	*buffer = ptr;
  smartfd_ptr->pos(*offset+(uint64_t)*length);
}


// PREAD
void
Client::pread(Filesystem::SmartFdPtr smartfd_ptr, size_t len, uint64_t offset,
	            bool verify_checksum, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_PREAD);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Pread params(smartfd_ptr->fd(), 
																		offset, len, verify_checksum);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending pread request at byte %llu on FS: %s",
													(Llu)offset, smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc)) {
		  open(smartfd_ptr);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}

size_t
Client::pread(Filesystem::SmartFdPtr smartfd_ptr, void *dst, size_t len,
	 					  uint64_t offset, bool verify_checksum) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	pread(smartfd_ptr, len, offset, verify_checksum, &sync_handler);
	
	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
		
		uint32_t length;
		uint64_t off;
		const void *data;
		decode_response_pread(smartfd_ptr, event, &data, &off, &length);
		HT_ASSERT(length <= len);
		memcpy(dst, data, length);
		return length;
	}
	catch (Exception &e) {
	  String e_desc = format("Error pread at byte %llu on FS: %s",
													(Llu)offset, smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc)) {
		  open(smartfd_ptr);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void Client::decode_response_pread(Filesystem::SmartFdPtr smartfd_ptr, 
	EventPtr &event, const void **buffer, uint64_t *offset, uint32_t *length) {
	decode_response_read(smartfd_ptr, event, buffer, offset, length);
}


// APPEND
void
Client::append(Filesystem::SmartFdPtr smartfd_ptr, StaticBuffer &buffer, 
	Flags flags, DispatchHandler *handler) {
	
	/*
	buffer.own = false;// else initial buffer deleted pn try again
	try_again: 
	// HT_INFOF("Client::append buffer ptr: %p, %s", (void *)buffer.base, smartfd_ptr->to_str().c_str());
	*/

	CommHeader header(Request::Handler::Factory::FUNCTION_APPEND);
	header.gid = smartfd_ptr->fd();
	header.alignment = HT_DIRECT_IO_ALIGNMENT; 
	CommBuf *cbuf = new CommBuf(header, HT_DIRECT_IO_ALIGNMENT, buffer);
	Request::Parameters::Append params(smartfd_ptr->fd(), 
																		 buffer.size, static_cast<uint8_t>(flags));
	uint8_t *base = (uint8_t *)cbuf->get_data_ptr();
	params.encode(cbuf->get_data_ptr_address());
	size_t padding = HT_DIRECT_IO_ALIGNMENT -
		(((uint8_t *)cbuf->get_data_ptr()) - base);
	memset(cbuf->get_data_ptr(), 0, padding);
	cbuf->advance_data_ptr(padding);
	CommBufPtr cbp(cbuf);

	try { send_message(cbp, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending append request %u bytes to FS: %s",
		                (unsigned)buffer.size, smartfd_ptr->to_str().c_str());
		/*
		if(e.code() != Error::FSBROKER_BAD_FILE_HANDLE && 
			 wait_for_connection(e.code(), e_desc)) {

			// wait for BAD_FILE_HANDLE error, open with WRITE|APPEND is required
		  open(smartfd_ptr); 
			goto try_again;
	  }
		*/
	  HT_THROW2(e.code(), e, e_desc);
	}
	//buffer.own = true;
}

size_t Client::append(Filesystem::SmartFdPtr smartfd_ptr, StaticBuffer &buffer, 
											Flags flags) {
	//try_again:

	DispatchHandlerSynchronizer sync_handler;
	append(smartfd_ptr, buffer, flags, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
		
		uint64_t offset;
		uint32_t amount;
		decode_response_append(smartfd_ptr, event, &offset, &amount);

		if (buffer.size != amount)
			HT_THROWF(Error::FSBROKER_IO_ERROR, "tried to append %u bytes but got %u,"
				" FS: %s" ,
				(unsigned)buffer.size, (unsigned)amount, smartfd_ptr->to_str().c_str());
		return (size_t)amount;
	}
	catch (Exception &e) {
	  String e_desc = format("Error appending %u bytes to FS: %s",
		                (unsigned)buffer.size, smartfd_ptr->to_str().c_str());
		/*
		if(e.code() != Error::FSBROKER_BAD_FILE_HANDLE && 
			 wait_for_connection(e.code(), e_desc)) {
		  
			// wait for BAD_FILE_HANDLE error, open with WRITE|APPEND is required
			open(smartfd_ptr);
			goto try_again;
	  }
		*/
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void Client::decode_response_append(Filesystem::SmartFdPtr smartfd_ptr, 
	EventPtr &event, uint64_t *offset, uint32_t *length) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Append params;
	params.decode(&ptr, &remain);
	*offset = params.get_offset();
	*length = params.get_amount();
  
  smartfd_ptr->pos(*offset+(uint64_t)*length);
}


// SEEK
void
Client::seek(Filesystem::SmartFdPtr smartfd_ptr, uint64_t offset, 
						 DispatchHandler *handler) {
	try_again:
	
	//HT_INFOF("Client::seek  %s %lu", smartfd_ptr->to_str().c_str(), offset);
	CommHeader header(Request::Handler::Factory::FUNCTION_SEEK);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Seek params(smartfd_ptr->fd(), offset);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending seek request to %llu on FS: %s", 
			(Llu)offset, smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc)) {
		  open(smartfd_ptr);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::seek(Filesystem::SmartFdPtr smartfd_ptr, uint64_t offset) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	seek(smartfd_ptr, offset, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
	}
	catch (Exception &e) {
	  String e_desc = format("Error seek to %llu on FS: %s", 
			(Llu)offset, smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc)) {
		  open(smartfd_ptr);
			goto try_again;
	  }
	  HT_THROW2(e.code(), e, e_desc);
	}
}


// FLUSH/SYNC
void
Client::flush(Filesystem::SmartFdPtr smartfd_ptr, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_FLUSH);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Flush params(smartfd_ptr->fd());
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  if (e.code() == Error::FSBROKER_BAD_FILE_HANDLE) // closed==flushed(java?)
			return;
	  String e_desc = format("Error sending flush request FS: %s", 
			smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc))
			goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::flush(Filesystem::SmartFdPtr smartfd_ptr) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	flush(smartfd_ptr, &sync_handler);

	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event)){
			HT_THROW(Protocol::response_code(event.get()), 
				Protocol::string_format_message(event));
		}
	}
	catch (Exception &e) {
	  if (e.code() == Error::FSBROKER_BAD_FILE_HANDLE)
			return;
	  String e_desc = format("Error flushing FS: %s", 
			smartfd_ptr->to_str().c_str());
	  if (wait_for_connection(e.code(), e_desc))
			goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::sync(Filesystem::SmartFdPtr smartfd_ptr) {
	try_again:
	DispatchHandlerSynchronizer sync_handler;
	CommHeader header(Request::Handler::Factory::FUNCTION_SYNC);
	header.gid = smartfd_ptr->fd();
	Request::Parameters::Sync params(smartfd_ptr->fd());
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());
	EventPtr event;

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()), 
							Protocol::string_format_message(event));
	}
	catch (Exception &e) {
	  String e_desc = format("Error syncing FS: %s", 
			smartfd_ptr->to_str().c_str());
	  if (e.code() != Error::FSBROKER_BAD_FILE_HANDLE 
				&& wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}



// Methods based on File/Path name

// REMOVE
void
Client::remove(const String &name, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_REMOVE);
	Request::Parameters::Remove params(name);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending remove request FS file: %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::remove(const String &name, bool force) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	remove(name, &sync_handler);

	try {
	  EventPtr event;
		if (!sync_handler.wait_for_reply(event)) {
			int error = Protocol::response_code(event.get());
			if (!force || error != Error::FSBROKER_FILE_NOT_FOUND)
				HT_THROW(error, Protocol::string_format_message(event).c_str());
		}
	}
	catch (Exception &e) {
	  String e_desc = format("Error removing FS file: %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}


// LENGTH
void Client::length(const String &name, bool accurate,
	DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_LENGTH);
	Request::Parameters::Length params(name, accurate);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending length request for FS file: %s", 
			name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

int64_t Client::length(const String &name, bool accurate) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	length(name, accurate, &sync_handler);
	
	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
		return decode_response_length(event);
	}
	catch (Exception &e) {
	  String e_desc = format("Error length for FS file: %s", 
			name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

int64_t Client::decode_response_length(EventPtr &event) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Length params;
	params.decode(&ptr, &remain);
	return params.get_length();
}


// MKDIRS
void Client::mkdirs(const String &name, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_MKDIRS);
	Request::Parameters::Mkdirs params(name);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending mkdirs request for FS "
			"directory: %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::mkdirs(const String &name) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	mkdirs(name, &sync_handler);

	try {
		EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
	  String e_desc = format("Error mkdirs for FS "
			"directory: %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}


// RMDIR
void Client::rmdir(const String &name, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_RMDIR);
	Request::Parameters::Rmdir params(name);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); } 
	catch (Exception &e) {
	  String e_desc = format("Error sending rmdir request for FS directory: "
			"%s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		   goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::rmdir(const String &name, bool force) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	rmdir(name, &sync_handler);

	try {
		EventPtr event;
		if (!sync_handler.wait_for_reply(event)) {
			int error = Protocol::response_code(event.get());
			if (!force || error != Error::FSBROKER_FILE_NOT_FOUND)
				HT_THROW(error, Protocol::string_format_message(event).c_str());
		}
	}
	catch (Exception &e) {
	  String e_desc = format("Error rmdir for FS directory: "
			"%s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		   goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}


// READDIR
void Client::readdir(const String &name, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_READDIR);
	Request::Parameters::Readdir params(name);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending readdir request for FS directory"
			": %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void Client::readdir(const String &name, std::vector<Dirent> &listing) {
	try_again:
	
	DispatchHandlerSynchronizer sync_handler;
	readdir(name, &sync_handler);
	
	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
		decode_response_readdir(event, listing);
	}
	catch (Exception &e) {
	  String e_desc = format("Error readdir for FS directory"
			": %s", name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void Client::decode_response_readdir(EventPtr &event,
	std::vector<Dirent> &listing) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Readdir params;
	params.decode(&ptr, &remain);
	params.get_listing(listing);
}


// EXISTS
void Client::exists(const String &name, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_EXISTS);
	Request::Parameters::Exists params(name);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending exists request for FS path: %s",
			name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

bool Client::exists(const String &name) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	exists(name, &sync_handler);

	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
		return decode_response_exists(event);
	}
	catch (Exception &e) {
	  String e_desc = format("Error  exists for FS path: %s",
			name.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

bool Client::decode_response_exists(EventPtr &event) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Exists params;
	params.decode(&ptr, &remain);
	return params.get_exists();
}


// RENAME
void
Client::rename(const String &src, const String &dst, DispatchHandler *handler) {
	try_again:
	CommHeader header(Request::Handler::Factory::FUNCTION_RENAME);
	Request::Parameters::Rename params(src, dst);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
	  String e_desc = format("Error sending 'rename' request for FS "
			"path: %s -> %s", src.c_str(), dst.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}

void
Client::rename(const String &src, const String &dst) {
	try_again:

	DispatchHandlerSynchronizer sync_handler;
	rename(src, dst, &sync_handler);

	try {
    EventPtr event;
		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
	  String e_desc = format("Error rename for FS "
			"path: %s -> %s", src.c_str(), dst.c_str());
	  if (wait_for_connection(e.code(), e_desc))
		  goto try_again;
	  HT_THROW2(e.code(), e, e_desc);
	}
}



// Methods based on int-fd, Depreciated FsBroker-Client methods -- for compatabillity to Common:FileSystem

// OPEN
[[deprecated("open(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::open(const String &name, uint32_t flags, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_OPEN);
	Request::Parameters::Open params(name, flags, 0);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, handler);
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error opening FS file: %s", name.c_str());
	}
}

int
Client::open(const String &name, uint32_t flags) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_OPEN);
	Request::Parameters::Open params(name, flags, 0);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		int32_t fd;
		decode_response_open(event, &fd);
		return fd;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error opening FS file: %s", name.c_str());
	}
}

[[deprecated("open_buffered(Filesystem::SmartFdPtr fd_obj,..)")]]
int
Client::open_buffered(const String &name, uint32_t flags, uint32_t buf_size,
	uint32_t outstanding, uint64_t start_offset,
	uint64_t end_offset) {
	try {
		HT_ASSERT((flags & Filesystem::OPEN_FLAG_DIRECTIO) == 0 ||
			(HT_IO_ALIGNED(buf_size) &&
				HT_IO_ALIGNED(start_offset) &&
				HT_IO_ALIGNED(end_offset)));
		int fd = open(name, flags | OPEN_FLAG_VERIFY_CHECKSUM);
		{
			lock_guard<mutex> lock(m_mutex);
			HT_ASSERT(m_buffered_reader_map.find(fd) == m_buffered_reader_map.end());
			
			ClientFdPtr cliendfd = std::make_shared<ClientFd>(
				name, flags|OPEN_FLAG_VERIFY_CHECKSUM, fd, 0);
			Filesystem::SmartFdPtr smartfd_ptr = 
				std::static_pointer_cast<Filesystem::SmartFd>(cliendfd);
    	cliendfd->reader = std::make_shared<ClientBufferedReaderHandler>(
          this, smartfd_ptr, buf_size, outstanding, start_offset, end_offset);
			// store in map with SmartFdPtr
			m_buffered_reader_map[fd] = smartfd_ptr;
		}
		return fd;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error opening buffered FS file=%s buf_size=%u "
			"outstanding=%u start_offset=%llu end_offset=%llu", name.c_str(),
			buf_size, outstanding, (Llu)start_offset, (Llu)end_offset);
	}
}

void Client::decode_response_open(EventPtr &event, int32_t *fd) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Open params;
	params.decode(&ptr, &remain);
	*fd = params.get_fd();
}


// CREATE
[[deprecated("create(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::create(const String &name, uint32_t flags, int32_t bufsz,
	int32_t replication, int64_t blksz,
	DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_CREATE);
	Request::Parameters::Create params(name, flags, bufsz, replication, blksz);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, handler);
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error creating FS file: %s:", name.c_str());
	}
}

[[deprecated("create(Filesystem::SmartFdPtr fd_obj,..)")]]
int
Client::create(const String &name, uint32_t flags, int32_t bufsz,
	int32_t replication, int64_t blksz) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_CREATE);
	Request::Parameters::Create params(name, flags, bufsz, replication, blksz);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		int32_t fd;
		decode_response_create(event, &fd);
		return fd;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error creating FS file: %s", name.c_str());
	}
}

void Client::decode_response_create(EventPtr &event, int32_t *fd) {
	decode_response_open(event, fd);
}


// READ
[[deprecated("read(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::read(int32_t fd, size_t len, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_READ);
	header.gid = fd;
	Request::Parameters::Read params(fd, len);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, handler);
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error sending read request for %u bytes "
			"from FS fd: %d", (unsigned)len, (int)fd);
	}
}

[[deprecated("read(Filesystem::SmartFdPtr fd_obj,..)")]]
size_t
Client::read(int32_t fd, void *dst, size_t len) {
	ClientFdPtr clientfd_ptr;
	{
		lock_guard<mutex> lock(m_mutex);
		auto iter = m_buffered_reader_map.find(fd);
		if (iter != m_buffered_reader_map.end())
   		clientfd_ptr = get_clientfd_ptr((*iter).second);
	}
	if (clientfd_ptr)
		return clientfd_ptr->reader->read(dst, len);
	try {

		DispatchHandlerSynchronizer sync_handler;
		EventPtr event;
		CommHeader header(Request::Handler::Factory::FUNCTION_READ);
		header.gid = fd;
		Request::Parameters::Read params(fd, len);
		CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
		params.encode(cbuf->get_data_ptr_address());
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		uint32_t length;
		uint64_t offset;
		const void *data;
		decode_response_read(event, &data, &offset, &length);
		HT_ASSERT(length <= len);
		memcpy(dst, data, length);
		return length;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error reading %u bytes from FS fd %d",
			(unsigned)len, (int)fd);
	}
}

void Client::decode_response_read(EventPtr &event, const void **buffer,
	uint64_t *offset, uint32_t *length) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Read params;
	params.decode(&ptr, &remain);
	*offset = params.get_offset();
	*length = params.get_amount();

	if (*length == (uint32_t)-1) {
		*length = 0;
		return;
	}

	if (remain < (size_t)*length)
		HT_THROWF(Error::RESPONSE_TRUNCATED, "%lu < %lu", (Lu)remain, (Lu)*length);

	*buffer = ptr;
}


// PREAD
[[deprecated("pread(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::pread(int32_t fd, size_t len, uint64_t offset,
	bool verify_checksum, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_PREAD);
	header.gid = fd;
	Request::Parameters::Pread params(fd, offset, len, verify_checksum);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error sending pread request at byte %llu "
			"on FS fd %d", (Llu)offset, (int)fd);
	}
}

[[deprecated("pread(Filesystem::SmartFdPtr fd_obj,..)")]]
size_t
Client::pread(int32_t fd, void *dst, size_t len, uint64_t offset, 
							bool verify_checksum) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_PREAD);
	header.gid = fd;
	Request::Parameters::Pread params(fd, offset, len, verify_checksum);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		uint32_t length;
		uint64_t off;
		const void *data;
		decode_response_pread(event, &data, &off, &length);
		HT_ASSERT(length <= len);
		memcpy(dst, data, length);
		return length;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error preading at byte %llu on FS fd %d",
			(Llu)offset, (int)fd);
	}
}

void Client::decode_response_pread(EventPtr &event, const void **buffer,
	uint64_t *offset, uint32_t *length) {
	decode_response_read(event, buffer, offset, length);
}


// APPEND
[[deprecated("append(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::append(int32_t fd, StaticBuffer &buffer, Flags flags,
	DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_APPEND);
	header.gid = fd;
	header.alignment = HT_DIRECT_IO_ALIGNMENT;
	CommBuf *cbuf = new CommBuf(header, HT_DIRECT_IO_ALIGNMENT, buffer);
	Request::Parameters::Append params(fd, buffer.size, 
																		 static_cast<uint8_t>(flags));
	uint8_t *base = (uint8_t *)cbuf->get_data_ptr();
	params.encode(cbuf->get_data_ptr_address());
	size_t padding = HT_DIRECT_IO_ALIGNMENT -
		(((uint8_t *)cbuf->get_data_ptr()) - base);
	memset(cbuf->get_data_ptr(), 0, padding);
	cbuf->advance_data_ptr(padding);

	CommBufPtr cbp(cbuf);

	try {
		send_message(cbp, handler);
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error appending %u bytes to FS fd %d",
			(unsigned)buffer.size, (int)fd);
	}
}

[[deprecated("append(Filesystem::SmartFdPtr fd_obj,..)")]]
size_t 
Client::append(int32_t fd, StaticBuffer &buffer, Flags flags) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;

	CommHeader header(Request::Handler::Factory::FUNCTION_APPEND);
	header.gid = fd;
	header.alignment = HT_DIRECT_IO_ALIGNMENT;
	CommBuf *cbuf = new CommBuf(header, HT_DIRECT_IO_ALIGNMENT, buffer);
	Request::Parameters::Append params(fd, buffer.size, static_cast<uint8_t>(flags));
	uint8_t *base = (uint8_t *)cbuf->get_data_ptr();
	params.encode(cbuf->get_data_ptr_address());
	size_t padding = HT_DIRECT_IO_ALIGNMENT -
		(((uint8_t *)cbuf->get_data_ptr()) - base);
	memset(cbuf->get_data_ptr(), 0, padding);
	cbuf->advance_data_ptr(padding);

	CommBufPtr cbp(cbuf);

	try {
		send_message(cbp, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());

		uint64_t offset;
		uint32_t amount;
		decode_response_append(event, &offset, &amount);

		if (buffer.size != amount)
			HT_THROWF(Error::FSBROKER_IO_ERROR, "tried to append %u bytes but got "
				"%u", (unsigned)buffer.size, (unsigned)amount);
		return (size_t)amount;
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error appending %u bytes to FS fd %d",
			(unsigned)buffer.size, (int)fd);
	}
}

void Client::decode_response_append(EventPtr &event, uint64_t *offset,
	uint32_t *length) {
	int error = Protocol::response_code(event);
	if (error != Error::OK)
		HT_THROW(error, Protocol::string_format_message(event));

	const uint8_t *ptr = event->payload + 4;
	size_t remain = event->payload_len - 4;

	Response::Parameters::Append params;
	params.decode(&ptr, &remain);
	*offset = params.get_offset();
	*length = params.get_amount();
}


// SEEK
[[deprecated("seek(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::seek(int32_t fd, uint64_t offset, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_SEEK);
	header.gid = fd;
	Request::Parameters::Seek params(fd, offset);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error seeking to %llu on FS fd %d",
			(Llu)offset, (int)fd);
	}
}

[[deprecated("seek(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::seek(int32_t fd, uint64_t offset) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_SEEK);
	header.gid = fd;
	Request::Parameters::Seek params(fd, offset);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error seeking to %llu on FS fd %d",
			(Llu)offset, (int)fd);
	}
}


// FLUSH/SYNC
[[deprecated("flush(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::flush(int32_t fd, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_FLUSH);
	header.gid = fd;
	Request::Parameters::Flush params(fd);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try { send_message(cbuf, handler); }
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error flushing FS fd %d", (int)fd);
	}
}

[[deprecated("flush(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::flush(int32_t fd) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_FLUSH);
	header.gid = fd;
	Request::Parameters::Flush params(fd);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error flushing FS fd %d", (int)fd);
	}
}

[[deprecated("sync(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::sync(int32_t fd) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_SYNC);
	header.gid = fd;
	Request::Parameters::Sync params(fd);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error syncing FS fd %d", (int)fd);
	}
}


[[deprecated("close(Filesystem::SmartFdPtr fd_obj,..)")]]
// CLOSE
void
Client::close(int32_t fd, DispatchHandler *handler) {
	CommHeader header(Request::Handler::Factory::FUNCTION_CLOSE);
	header.gid = fd;
	Request::Parameters::Close params(fd);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());

	{
		lock_guard<mutex> lock(m_mutex);
		auto iter = m_buffered_reader_map.find(fd);
		if (iter != m_buffered_reader_map.end()) {
			// Filesystem::SmartFdPtr smartfd_ptr = (*iter).second;
			// ptr delete smartfd_ptr->reader
			m_buffered_reader_map.erase(iter);
		}
	}

	try {
		send_message(cbuf, handler);
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error closing FS fd: %d", (int)fd);
	}
}

[[deprecated("close(Filesystem::SmartFdPtr fd_obj,..)")]]
void
Client::close(int32_t fd) {
	DispatchHandlerSynchronizer sync_handler;
	EventPtr event;
	CommHeader header(Request::Handler::Factory::FUNCTION_CLOSE);
	header.gid = fd;
	Request::Parameters::Close params(fd);
	CommBufPtr cbuf(new CommBuf(header, params.encoded_length()));
	params.encode(cbuf->get_data_ptr_address());
	{
		lock_guard<mutex> lock(m_mutex);
		auto iter = m_buffered_reader_map.find(fd);
		if (iter != m_buffered_reader_map.end()) {
			// Filesystem::SmartFdPtr smartfd_ptr = (*iter).second;
			// ptr delete smartfd_ptr->reader
			m_buffered_reader_map.erase(iter);
		}
	}

	try {
		send_message(cbuf, &sync_handler);

		if (!sync_handler.wait_for_reply(event))
			HT_THROW(Protocol::response_code(event.get()),
				Protocol::string_format_message(event).c_str());
	}
	catch (Exception &e) {
		HT_THROW2F(e.code(), e, "Error closing FS fd: %d", (int)fd);
	}
}

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

/** @file
 * Definitions for ConnectionManager.
 * This file contains method definitions for ConnectionManager, a class for
 * establishing and maintaining TCP connections.
 */

#include <Common/Compat.h>
#include "ConnectionManager.h"

#include <AsyncComm/Comm.h>
#include <AsyncComm/Protocol.h>

#include <Common/Error.h>
#include <Common/Logger.h>
#include <Common/Random.h>
#include <Common/Serialization.h>
#include <Common/System.h>
#include <Common/Time.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

extern "C" {
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
}

using namespace Hypertable;
using namespace std;

void
ConnectionManager::add(const CommAddress &addr, uint32_t timeout_ms,
                       const char *service_name, DispatchHandlerPtr &handler) {
  CommAddress null_addr;
  ConnectionInitializerPtr null_initializer;
  add_internal(addr, null_addr, timeout_ms, service_name, handler, null_initializer);
}

void ConnectionManager::add_with_initializer(const CommAddress &addr,
    uint32_t timeout_ms, const char *service_name,
    DispatchHandlerPtr &handler, ConnectionInitializerPtr &initializer) {
  CommAddress null_addr;
  add_internal(addr, null_addr, timeout_ms, service_name, handler, initializer);
}


void
ConnectionManager::add(const CommAddress &addr, uint32_t timeout_ms,
                       const char *service_name) {
  DispatchHandlerPtr null_disp_handler;
  add(addr, timeout_ms, service_name, null_disp_handler);
}


void
ConnectionManager::add(const CommAddress &addr, const CommAddress &local_addr,
                       uint32_t timeout_ms, const char *service_name,
                       DispatchHandlerPtr &handler) {
  ConnectionInitializerPtr null_initializer;
  add_internal(addr, local_addr, timeout_ms, service_name, handler, null_initializer);
}


void
ConnectionManager::add(const CommAddress &addr, const CommAddress &local_addr,
                       uint32_t timeout_ms, const char *service_name) {
  DispatchHandlerPtr null_disp_handler;
  add(addr, local_addr, timeout_ms, service_name, null_disp_handler);
}

void
ConnectionManager::add_internal(const CommAddress &addr,
          const CommAddress &local_addr, uint32_t timeout_ms,
          const char *service_name, DispatchHandlerPtr &handler,
          ConnectionInitializerPtr &initializer) {
  lock_guard<mutex> lock(m_impl->mutex);
  ConnectionStatePtr conn_state;

  /// Start retry thread
  if (!m_impl->thread.joinable())
    m_impl->thread = std::thread([=](){ this->connect_retry_loop(); });

  HT_ASSERT(addr.is_set());

  if (addr.is_proxy()) {
    auto iter = m_impl->conn_map_proxy.find(addr.proxy);
    if (iter != m_impl->conn_map_proxy.end() && iter->second->state != State::DECOMMISSIONED)
      return;
  }
  else if (addr.is_inet()) {
    SockAddrMap<ConnectionStatePtr>::iterator iter = 
      m_impl->conn_map.find(addr.inet);
    if (iter != m_impl->conn_map.end() && iter->second->state != State::DECOMMISSIONED)
      return;
  }

  conn_state = make_shared<ConnectionState>();
  conn_state->addr = addr;
  conn_state->local_addr = local_addr;
  conn_state->timeout_ms = timeout_ms;
  conn_state->handler = handler;
  conn_state->initializer = initializer;
  conn_state->service_name = (service_name) ? service_name : "";
  conn_state->next_retry = std::chrono::steady_clock::now();

  if (addr.is_proxy())
    m_impl->conn_map_proxy[addr.proxy] = conn_state;
  else
    m_impl->conn_map[addr.inet] = conn_state;

  {
    lock_guard<mutex> conn_lock(conn_state->mutex);
    send_connect_request(conn_state);
  }
}


bool
ConnectionManager::wait_for_connection(const CommAddress &addr,
                                       uint32_t max_wait_ms) {
  Timer timer(max_wait_ms, true);
  return wait_for_connection(addr, timer);
}


bool
ConnectionManager::wait_for_connection(const CommAddress &addr,
                                       Timer &timer) {
  ConnectionStatePtr conn_state_ptr;

  {
    lock_guard<mutex> lock(m_impl->mutex);
    if (addr.is_inet()) {
      SockAddrMap<ConnectionStatePtr>::iterator iter =
	m_impl->conn_map.find(addr.inet);
      if (iter == m_impl->conn_map.end())
	return false;
      conn_state_ptr = (*iter).second;
    }
    else if (addr.is_proxy()) {
      auto iter = m_impl->conn_map_proxy.find(addr.proxy);
      if (iter == m_impl->conn_map_proxy.end())
	return false;
      conn_state_ptr = (*iter).second;
    }
  }
  
  return wait_for_connection(conn_state_ptr, timer);
}

bool
ConnectionManager::is_connection_state(const CommAddress &addr, State state) {
  lock_guard<mutex> lock(m_impl->mutex);
  if (addr.is_inet()) {
    SockAddrMap<ConnectionStatePtr>::iterator iter =
	  m_impl->conn_map.find(addr.inet);
    if (iter == m_impl->conn_map.end())
	    return false;
    return (*iter).second->state == state;
  }
  else if (addr.is_proxy()) {
    auto iter = m_impl->conn_map_proxy.find(addr.proxy);
    if (iter == m_impl->conn_map_proxy.end())
	    return false;
    return (*iter).second->state == state;
  }
  return false;
}

bool
ConnectionManager::is_addr_exists(const CommAddress &addr) {
  lock_guard<mutex> lock(m_impl->mutex);
  if (addr.is_inet()) {
    SockAddrMap<ConnectionStatePtr>::iterator iter =
	  m_impl->conn_map.find(addr.inet);
    return iter != m_impl->conn_map.end();
  }
  else if (addr.is_proxy()) {
    auto iter = m_impl->conn_map_proxy.find(addr.proxy);
    return iter != m_impl->conn_map_proxy.end();
  }
  return false;
}

bool ConnectionManager::wait_for_connection(ConnectionStatePtr &conn_state,
					    Timer &timer) {

  // if (conn_state->state == State::READY)
  //	return true;
  timer.start();

  {
    unique_lock<mutex> conn_lock(conn_state->mutex);

    auto duration = std::chrono::milliseconds(timer.remaining());

    if (!conn_state->cond.wait_for(conn_lock, duration, [&conn_state](){ return conn_state->state == State::READY; }))
      return false;

    if (conn_state->state == State::DECOMMISSIONED)
      return false;

  }

  return true;
}


/**
 * Attempts to establish a connection for the given ConnectionState object.  If
 * a failure occurs, it prints an error message and then schedules a retry by
 * updating the next_retry member of the conn_state object and pushing it onto
 * the retry heap
 *
 * @param conn_state The connection state record
 */
void
ConnectionManager::send_connect_request(ConnectionStatePtr &conn_state) {
  int error;

  if (conn_state->state == State::DECOMMISSIONED)
    HT_FATALF("Attempt to connect decommissioned connection to service='%s'",
              conn_state->service_name.c_str());

  if (!conn_state->local_addr.is_set())
    error = m_impl->comm->connect(conn_state->addr, shared_from_this());
  else
    error = m_impl->comm->connect(conn_state->addr, conn_state->local_addr,
                                  shared_from_this());

  if (error == Error::OK)
    return;
  else if (error == Error::COMM_ALREADY_CONNECTED) {
    if (conn_state->state == State::DISCONNECTED)
      conn_state->state = State::READY;
    conn_state->cond.notify_all();
  }
  else if (error == Error::COMM_INVALID_PROXY) {
    m_impl->conn_map.erase(conn_state->inet_addr);
    m_impl->conn_map_proxy.erase(conn_state->addr.proxy);
    conn_state->state = State::DECOMMISSIONED;
    conn_state->cond.notify_all();
  }
  else if (error != Error::COMM_BROKEN_CONNECTION) {
    if (!m_impl->quiet_mode) {
      if (conn_state->service_name != "")
        HT_INFOF("Connection attempt to %s at %s failed - %s.  Will retry "
                 "again in %d milliseconds...", conn_state->service_name.c_str(),
                 conn_state->addr.to_str().c_str(), Error::get_text(error),
                 (int)conn_state->timeout_ms);
      else
        HT_INFOF("Connection attempt to service at %s failed - %s.  Will retry "
                 "again in %d milliseconds...", conn_state->addr.to_str().c_str(),
                 Error::get_text(error), (int)conn_state->timeout_ms);
    }

    // Reschedule (throw in a little randomness)
    conn_state->next_retry = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(conn_state->timeout_ms);

    if (Random::number32() & 1)
      conn_state->next_retry -= Random::duration_millis(2000);
    else
      conn_state->next_retry += Random::duration_millis(2000);

    // add to retry heap
    m_impl->retry_queue.push(conn_state);
    m_impl->retry_cond.notify_one();
  }

}


int ConnectionManager::remove(const CommAddress &addr) {
  bool check_inet_addr = false;
  InetAddr inet_addr;
  bool do_close = false;
  int error = Error::OK;

  HT_ASSERT(addr.is_set());

  {
    lock_guard<mutex> lock(m_impl->mutex);

    if (addr.is_proxy()) {
      auto iter = m_impl->conn_map_proxy.find(addr.proxy);
      if (iter != m_impl->conn_map_proxy.end()) {
	{
	  lock_guard<mutex> conn_lock((*iter).second->mutex);
	  check_inet_addr = true;
	  inet_addr = (*iter).second->inet_addr;
          if ((*iter).second->state == State::CONNECTED ||
              (*iter).second->state == State::READY)
            do_close = true;
	  (*iter).second->state = State::DECOMMISSIONED;
          (*iter).second->cond.notify_all();
	}
	m_impl->conn_map_proxy.erase(iter);
      }
    }
    else if (addr.is_inet()) {
      check_inet_addr = true;
      inet_addr = addr.inet;
    }

    if (check_inet_addr) {
      SockAddrMap<ConnectionStatePtr>::iterator iter =
        m_impl->conn_map.find(inet_addr);
      if (iter != m_impl->conn_map.end()) {
	{
	  lock_guard<mutex> conn_lock((*iter).second->mutex);
          if ((*iter).second->state == State::CONNECTED ||
              (*iter).second->state == State::READY)
            do_close = true;
	  (*iter).second->state = State::DECOMMISSIONED;
          (*iter).second->cond.notify_all();
	}
	m_impl->conn_map.erase(iter);
      }
      
    }
  }

  if (do_close)
    m_impl->comm->close_socket(addr);

  return error;
}



/**
 * This is the AsyncComm dispatch handler method.  It gets called for each
 * connection related event (establishment, disconnect, etc.) for each
 * connection.  For connect events, the connection's connected flag is set to
 * true and it's condition variable is signaled.  For all other events (e.g.
 * disconnect or error), the connection's connected flag is set to false and a
 * retry is scheduled.
 *
 * @param event shared pointer to event object
 */
void
ConnectionManager::handle(EventPtr &event) {
  lock_guard<mutex> lock(m_impl->mutex);
  ConnectionStatePtr conn_state;

  {
    auto iter = m_impl->conn_map.find(event->addr);
    if (iter != m_impl->conn_map.end())
      conn_state = (*iter).second;
  }

  if (!conn_state && event->proxy) {
    auto iter = m_impl->conn_map_proxy.find(event->proxy);
    if (iter != m_impl->conn_map_proxy.end()) {
      conn_state = (*iter).second;
      /** register address **/
      m_impl->conn_map[event->addr] = conn_state;
    }
  }

  if (conn_state) {
    lock_guard<mutex> conn_lock(conn_state->mutex);

    if (event->type == Event::CONNECTION_ESTABLISHED) {
      conn_state->inet_addr = event->addr;
      if (conn_state->initializer) {
        conn_state->state = State::CONNECTED;
        send_initialization_request(conn_state);
        return;
      }
      else {
        conn_state->state = State::READY;
        conn_state->cond.notify_all();
      }
    }
    else if (event->type == Event::ERROR ||
             event->type == Event::DISCONNECT) {
      if (event->proxy && !m_impl->comm->translate_proxy(event->proxy, 0)) {
        m_impl->conn_map.erase(conn_state->inet_addr);
        m_impl->conn_map_proxy.erase(conn_state->addr.proxy);
        conn_state->state = State::DECOMMISSIONED;
        conn_state->cond.notify_all();
      }
      else {
        if (!m_impl->quiet_mode)
          HT_INFOF("Received event %s", event->to_str().c_str());
        string message = (event->type == Event::DISCONNECT) ?
          "Disconnected" : Error::get_text(event->error);
        conn_state->state = State::DISCONNECTED;
        schedule_retry(conn_state, message);
      }
    }
    else if (event->type == Event::MESSAGE) {
      if (conn_state->initializer && conn_state->state == State::CONNECTED) {
        if (Protocol::response_code(event) == Error::SERVER_NOT_READY) {
          schedule_retry(conn_state, Error::get_text(Error::SERVER_NOT_READY));
          return;
        }
        else if (event->header.command != conn_state->initializer->initialization_command()) {
          String err_msg = "Connection initialization not yet complete";
          CommHeader header;
          header.initialize_from_request_header(event->header);
          CommBufPtr cbuf( new CommBuf(header, 4 + Serialization::encoded_length_str16(err_msg)) );
          cbuf->append_i32(Error::CONNECTION_NOT_INITIALIZED);
          cbuf->append_str16(err_msg);
          m_impl->comm->send_response(event->addr, cbuf);
          return;
        }
        if (!conn_state->initializer->process_initialization_response(event.get()))
          HT_FATALF("Unable to initialize connection to %s, exiting ...",
                    conn_state->service_name.c_str());
        conn_state->state = State::READY;
        conn_state->cond.notify_all();
        return;
      }
    }

    // Chain event to application supplied handler
    if (conn_state->handler)
      conn_state->handler->handle(event);
  }
  else {
    HT_WARNF("Unable to find connection for %s in map.",
             InetAddr::format(event->addr).c_str());
  }
}

void ConnectionManager::send_initialization_request(ConnectionStatePtr &conn_state) {
  CommBufPtr cbuf(conn_state->initializer->create_initialization_request());
  int error = m_impl->comm->send_request(conn_state->inet_addr, 60000, cbuf, this);
  if (error == Error::COMM_BROKEN_CONNECTION ||
      error == Error::COMM_NOT_CONNECTED ||
      error == Error::COMM_INVALID_PROXY) {
    if (!m_impl->quiet_mode)
      HT_INFOF("Received error %d", error);
    conn_state->state = State::DISCONNECTED;
    schedule_retry(conn_state, Error::get_text(error));
  }
  else if (error != Error::OK)
    HT_FATALF("Problem initializing connection to %s - %s", 
              conn_state->service_name.c_str(), Error::get_text(error));
}


void ConnectionManager::schedule_retry(ConnectionStatePtr &conn_state,
                                       const string &message) {
  if (!m_impl->quiet_mode)
    HT_INFOF("%s: Problem connecting to %s, will retry in %d "
             "milliseconds...", message.c_str(),
             conn_state->service_name.c_str(), (int)conn_state->timeout_ms);

  // this logic could proably be smarter.  For example, if the last
  // connection attempt was a long time ago, then schedule immediately
  // otherwise, if this event is the result of an immediately prior connect
  // attempt, then do the following
  conn_state->next_retry = std::chrono::steady_clock::now() +
    std::chrono::milliseconds(conn_state->timeout_ms);

  // add to retry heap
  m_impl->retry_queue.push(conn_state);
  m_impl->retry_cond.notify_one();
}


/**
 * This is the boost::thread run method.
 */
void ConnectionManager::connect_retry_loop() {
  unique_lock<mutex> lock(m_impl->mutex);
  ConnectionStatePtr conn_state;

  while (!m_impl->shutdown) {

    while (m_impl->retry_queue.empty()) {
      m_impl->retry_cond.wait(lock);
      if (m_impl->shutdown)
        break;
    }

    if (m_impl->shutdown)
      break;

    conn_state = m_impl->retry_queue.top();

    {
      lock_guard<mutex> conn_lock(conn_state->mutex);
      if (conn_state->state == State::DISCONNECTED) {
        if (conn_state->next_retry <= std::chrono::steady_clock::now()) {
          m_impl->retry_queue.pop();
          send_connect_request(conn_state);
          continue;
        }
      }
      else if (conn_state->state == State::CONNECTED && conn_state->initializer) {
        if (conn_state->next_retry <= std::chrono::steady_clock::now()) {
          m_impl->retry_queue.pop();
          send_initialization_request(conn_state);
          continue;
        }
      }
      else {
        m_impl->retry_queue.pop();
        continue;
      }
    }
    m_impl->retry_cond.wait_until(lock, conn_state->next_retry);
  }
}



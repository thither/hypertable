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
 * Definitions for IOHandlerData.
 * This file contains method definitions for IOHandlerData, a class for
 * processing I/O events for data (TCP) sockets.
 */

#include <Common/Compat.h>

#include "IOHandlerData.h"
#include "ReactorRunner.h"

#include <Common/Error.h>
#include <Common/FileUtils.h>
#include <Common/InetAddr.h>
#include <Common/Time.h>

#include <cassert>
#include <chrono>
#include <iostream>

extern "C" {
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/event.h>
#endif
#include <sys/uio.h>
}

using namespace Hypertable;
using namespace std;


namespace {

  /**
   * Used to read data off a socket that is monotored with edge-triggered epoll.
   * When this function returns with *errnop set to EAGAIN, it is safe to call
   * epoll_wait on this socket.
   */
  ssize_t
  et_socket_read(int fd, void *vptr, size_t n, int *errnop, bool *eofp) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = (char *)vptr;

    while (nleft > 0) {
      if ((nread = ::read(fd, ptr, nleft)) < 0) {
        if (errno == EINTR) {
          nread = 0; /* and call read() again */
          continue;
        }
        *errnop = errno;

        if (*errnop == EAGAIN || nleft < n)
          break;     /* already read something, most likely EAGAIN */

        return -1;   /* other errors and nothing read */
      }
      else if (nread == 0) {
        *eofp = true;
        break;
      }

      nleft -= nread;
      ptr   += nread;
    }
    return n - nleft;
  }

  ssize_t
  et_socket_writev(int fd, const iovec *vector, int count, int *errnop) {
    ssize_t nwritten;
    while ((nwritten = writev(fd, vector, count)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0; /* and call write() again */
        continue;
      }
      *errnop = errno;
      return -1;
    }
    return nwritten;
  }

} // local namespace


bool
IOHandlerData::handle_event(struct pollfd *event,
                            ClockT::time_point arrival_time) {
  int error = 0;
  bool eof = false;

  //DisplayEvent(event);

  try {
    if (event->revents & POLLOUT) {
      if (handle_write_readiness()) {
        handle_disconnect();
        return true;
      }
    }

    if (event->revents & POLLIN) {
      size_t nread;
      while (true) {
        if (!m_got_header) {
          nread = et_socket_read(m_sd, m_message_header_ptr,
                                 m_message_header_remaining, &error, &eof);
          if (nread == (size_t)-1) {
            if (errno != ECONNREFUSED) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                         (int)m_message_header_remaining, strerror(errno));
            }
            else
              test_and_set_error(Error::COMM_CONNECT_ERROR);

            handle_disconnect();
            return true;
          }
          else if (nread < m_message_header_remaining) {
            m_message_header_remaining -= nread;
            m_message_header_ptr += nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else {
            m_message_header_ptr += nread;
            handle_message_header(arrival_time);
          }

          if (eof)
            break;
        }
        else { // got header
          nread = et_socket_read(m_sd, m_message_ptr, m_message_remaining,
                                 &error, &eof);
          if (nread == (size_t)-1) {
            if (ReactorFactory::verbose->get())
              HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                       (int)m_message_header_remaining, strerror(errno));
            handle_disconnect();
            return true;
          }
          else if (nread < m_message_remaining) {
            m_message_ptr += nread;
            m_message_remaining -= nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else
            handle_message_body();

          if (eof)
            break;
        }
      }
    }

    if (eof) {
      HT_DEBUGF("Received EOF on descriptor %d (%s:%d)", m_sd,
		inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    if (event->revents & POLLERR) {
      if (ReactorFactory::verbose->get())
        HT_INFOF("Received POLLERR on descriptor %d (%s:%d)", m_sd,
                 inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    if (event->revents & POLLHUP) {
      HT_DEBUGF("Received POLLHUP on descriptor %d (%s:%d)", m_sd,
		inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    HT_ASSERT((event->revents & POLLNVAL) == 0);

  }
  catch (Hypertable::Exception &e) {
    if (ReactorFactory::verbose->get())
      HT_ERROR_OUT << e << HT_END;
    handle_disconnect();
    return true;
  }

  return false;
}

#if defined(__linux__)

bool
IOHandlerData::handle_event(struct epoll_event *event,
                            ClockT::time_point arrival_time) {
  int error = 0;
  bool eof = false;

  //DisplayEvent(event);

  try {
    if (event->events & EPOLLOUT) {
      if (handle_write_readiness()) {
        handle_disconnect();
        return true;
      }
    }

    if (event->events & EPOLLIN) {
      size_t nread;
      while (true) {
        if (!m_got_header) {
          nread = et_socket_read(m_sd, m_message_header_ptr,
                                 m_message_header_remaining, &error, &eof);
          if (nread == (size_t)-1) {
            if (errno != ECONNREFUSED) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                         (int)m_message_header_remaining, strerror(errno));
            }
            else
              test_and_set_error(Error::COMM_CONNECT_ERROR);

            handle_disconnect();
            return true;
          }
          else if (nread < m_message_header_remaining) {
            m_message_header_remaining -= nread;
            m_message_header_ptr += nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else {
            m_message_header_ptr += nread;
            handle_message_header(arrival_time);
          }

          if (eof)
            break;
        }
        else { // got header
          nread = et_socket_read(m_sd, m_message_ptr, m_message_remaining,
                                 &error, &eof);
          if (nread == (size_t)-1) {
            if (ReactorFactory::verbose->get())
              HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                       (int)m_message_header_remaining, strerror(errno));
            handle_disconnect();
            return true;
          }
          else if (nread < m_message_remaining) {
            m_message_ptr += nread;
            m_message_remaining -= nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else
            handle_message_body();

          if (eof)
            break;
        }
      }
    }

    if (ReactorFactory::ms_epollet) {
      if (event->events & POLLRDHUP) {
        HT_DEBUGF("Received POLLRDHUP on descriptor %d (%s:%d)", m_sd,
                  inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
        handle_disconnect();
        return true;
      }
    }
    else {
      if (eof) {
        HT_DEBUGF("Received EOF on descriptor %d (%s:%d)", m_sd,
                  inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
        handle_disconnect();
        return true;
      }
    }

    if (event->events & EPOLLERR) {
      if (ReactorFactory::verbose->get())
        HT_INFOF("Received EPOLLERR on descriptor %d (%s:%d)", m_sd,
                 inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    if (event->events & EPOLLHUP) {
      HT_DEBUGF("Received EPOLLHUP on descriptor %d (%s:%d)", m_sd,
                inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }
  }
  catch (Hypertable::Exception &e) {
    if (ReactorFactory::verbose->get())
      HT_ERROR_OUT << e << HT_END;
    handle_disconnect();
    return true;
  }

  return false;
}

#elif defined(__sun__)

bool IOHandlerData::handle_event(port_event_t *event,
                                 ClockT::time_point arrival_time) {
  int error = 0;
  bool eof = false;

  //display_event(event);

  try {

    if (event->portev_events & POLLOUT) {
      if (handle_write_readiness()) {
        if (ReactorFactory::verbose->get())
          HT_INFO("handle_disconnect() write readiness");
        handle_disconnect();
        return true;
      }
    }

    if (event->portev_events & POLLIN) {
      size_t nread;
      while (true) {
        if (!m_got_header) {
          nread = et_socket_read(m_sd, m_message_header_ptr,
                                 m_message_header_remaining, &error, &eof);
          if (nread == (size_t)-1) {
            if (errno != ECONNREFUSED) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                         (int)m_message_header_remaining, strerror(errno));
            }
            else
              test_and_set_error(Error::COMM_CONNECT_ERROR);

            handle_disconnect();
            return true;
          }
          else if (nread < m_message_header_remaining) {
            m_message_header_remaining -= nread;
            m_message_header_ptr += nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else {
            m_message_header_ptr += nread;
            handle_message_header(arrival_time);
          }

          if (eof)
            break;
        }
        else { // got header
          nread = et_socket_read(m_sd, m_message_ptr, m_message_remaining,
                                 &error, &eof);
          if (nread == (size_t)-1) {
            if (ReactorFactory::verbose->get())
              HT_INFOF("socket read(%d, len=%d) failure : %s", m_sd,
                       (int)m_message_header_remaining, strerror(errno));
            handle_disconnect();
            return true;
          }
          else if (nread < m_message_remaining) {
            m_message_ptr += nread;
            m_message_remaining -= nread;
            if (error == EAGAIN)
              break;
            error = 0;
          }
          else
            handle_message_body();

          if (eof)
            break;
        }
      }
    }

    if (eof) {
      HT_DEBUGF("Received EOF on descriptor %d (%s:%d)", m_sd,
		inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }


    if (event->portev_events & POLLERR) {
      if (ReactorFactory::verbose->get())
        HT_INFOF("Received POLLERR on descriptor %d (%s:%d)", m_sd,
                 inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    if (event->portev_events & POLLHUP) {
      HT_DEBUGF("Received POLLHUP on descriptor %d (%s:%d)", m_sd,
                inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }

    if (event->portev_events & POLLREMOVE) {
      HT_DEBUGF("Received POLLREMOVE on descriptor %d (%s:%d)", m_sd,
                inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
      handle_disconnect();
      return true;
    }
    
  }
  catch (Hypertable::Exception &e) {
    if (ReactorFactory::verbose->get())
      HT_ERROR_OUT << e << HT_END;
    test_and_set_error(e.code());
    handle_disconnect();
    return true;
  }

  return false;
}

#elif defined(__APPLE__) || defined(__FreeBSD__)

bool IOHandlerData::handle_event(struct kevent *event,
                                 ClockT::time_point arrival_time) {

  //DisplayEvent(event);

  try {
    assert(m_sd == (int)event->ident);

    if (event->flags & EV_EOF) {
      handle_disconnect();
      return true;
    }

    if (event->filter == EVFILT_WRITE) {
      if (handle_write_readiness()) {
        handle_disconnect();
        return true;
      }
    }

    if (event->filter == EVFILT_READ) {
      size_t available = (size_t)event->data;
      size_t nread;
      while (available > 0) {
        if (!m_got_header) {
          if (m_message_header_remaining <= available) {
            nread = FileUtils::read(m_sd, m_message_header_ptr,
                                    m_message_header_remaining);
            if (nread == (size_t)-1) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("FileUtils::read(%d, len=%d) failure : %s", m_sd,
                         (int)m_message_header_remaining, strerror(errno));
              handle_disconnect();
              return true;
            }
            assert(nread == m_message_header_remaining);
            available -= nread;
            m_message_header_ptr += nread;
            handle_message_header(arrival_time);
          }
          else {
            nread = FileUtils::read(m_sd, m_message_header_ptr, available);
            if (nread == (size_t)-1) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("FileUtils::read(%d, len=%d) failure : %s", m_sd,
                         (int)available, strerror(errno));
              handle_disconnect();
              return true;
            }
            assert(nread == available);
            m_message_header_remaining -= nread;
            m_message_header_ptr += nread;
            return false;
          }
        }
        if (m_got_header) {
          if (m_message_remaining <= available) {
            nread = FileUtils::read(m_sd, m_message_ptr, m_message_remaining);
            if (nread == (size_t)-1) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("FileUtils::read(%d, len=%d) failure : %s", m_sd,
                         (int)m_message_remaining, strerror(errno));
              handle_disconnect();
              return true;
            }
            assert(nread == m_message_remaining);
            available -= nread;
            handle_message_body();
          }
          else {
            nread = FileUtils::read(m_sd, m_message_ptr, available);
            if (nread == (size_t)-1) {
              if (ReactorFactory::verbose->get())
                HT_INFOF("FileUtils::read(%d, len=%d) failure : %s", m_sd,
                         (int)available, strerror(errno));
              handle_disconnect();
              return true;
            }
            assert(nread == available);
            m_message_ptr += nread;
            m_message_remaining -= nread;
            available = 0;
          }
        }
      }
    }
  }
  catch (Hypertable::Exception &e) {
    if (ReactorFactory::verbose->get())
      HT_ERROR_OUT << e << HT_END;
    test_and_set_error(e.code());
    handle_disconnect();
    return true;
  }

  return false;
}
#else
  ImplementMe;
#endif


void IOHandlerData::handle_message_header(ClockT::time_point arrival_time) {
  size_t header_len = (size_t)m_message_header[1];

  // check to see if there is any variable length header
  // after the fixed length portion that needs to be read
  if (header_len > (size_t)(m_message_header_ptr - m_message_header)) {
    m_message_header_remaining = header_len - (size_t)(m_message_header_ptr
                                                       - m_message_header);
    return;
  }

  m_event = make_shared<Event>(Event::MESSAGE, m_addr);
  m_event->load_message_header(m_message_header, header_len);
  m_event->arrival_time = arrival_time;

  m_message_aligned = false;

#if defined(__linux__)
  if (m_event->header.alignment > 0) {
    void *vptr = 0;
    posix_memalign(&vptr, m_event->header.alignment,
		   m_event->header.total_len - header_len);
    m_message = (uint8_t *)vptr;
    m_message_aligned = true;
  }
  else
    m_message = new uint8_t [m_event->header.total_len - header_len];
#else
  m_message = new uint8_t [m_event->header.total_len - header_len];
#endif
  m_message_ptr = m_message;
  m_message_remaining = m_event->header.total_len - header_len;
  m_message_header_remaining = 0;
  m_got_header = true;
}


void IOHandlerData::handle_message_body() {
  DispatchHandler *dh {};

  if (m_event->header.flags & CommHeader::FLAGS_BIT_PROXY_MAP_UPDATE) {
    ReactorRunner::handler_map->update_proxy_map((const char *)m_message,
                  m_event->header.total_len - m_event->header.header_len);
    free_message_buffer();
    m_event.reset();
    //HT_INFO("proxy map update");
  }
  else if ((m_event->header.flags & CommHeader::FLAGS_BIT_REQUEST) == 0 &&
           (m_event->header.id == 0
            || !m_reactor->remove_request(m_event->header.id, dh))) {
    if ((m_event->header.flags & CommHeader::FLAGS_BIT_IGNORE_RESPONSE) == 0) {
      if (ReactorFactory::verbose->get())
        HT_WARNF("Received response for non-pending event (id=%d,version"
                 "=%d,total_len=%d)", m_event->header.id, m_event->header.version,
                 m_event->header.total_len);
    }
    free_message_buffer();
    m_event.reset();
  }
  else {
    m_event->payload = m_message;
    m_event->payload_len = m_event->header.total_len
                           - m_event->header.header_len;
    m_event->payload_aligned = m_message_aligned;
    {
      lock_guard<mutex> lock(m_mutex);
      m_event->set_proxy(m_proxy);
    }
    //HT_INFOF("Just received messaage of size %d", m_event->header.total_len);
    deliver_event(m_event, dh);  
  }

  reset_incoming_message_state();
}

void IOHandlerData::handle_disconnect() {
  ReactorRunner::handler_map->decomission_handler(this);
}

bool IOHandlerData::handle_write_readiness() {
  bool deliver_conn_estab_event = false;
  bool rval = true;
  int error = Error::OK;

  while (true) {
    lock_guard<mutex> lock(m_mutex);

    if (m_connected == false) {
      socklen_t name_len = sizeof(m_local_addr);
      int sockerr = 0;
      socklen_t sockerr_len = sizeof(sockerr);

      if (getsockopt(m_sd, SOL_SOCKET, SO_ERROR, &sockerr, &sockerr_len) < 0) {
        if (ReactorFactory::verbose->get())
          HT_INFOF("getsockopt(SO_ERROR) failed - %s", strerror(errno));
      }

      if (sockerr) {
        if (ReactorFactory::verbose->get())
          HT_INFOF("connect() completion error - %s", strerror(sockerr));
	break;
      }

      int bufsize = 4*32768;
      if (setsockopt(m_sd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
		     sizeof(bufsize)) < 0) {
        if (ReactorFactory::verbose->get())
          HT_INFOF("setsockopt(SO_SNDBUF) failed - %s", strerror(errno));
      }
      if (setsockopt(m_sd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
		     sizeof(bufsize)) < 0) {
        if (ReactorFactory::verbose->get())
          HT_INFOF("setsockopt(SO_RCVBUF) failed - %s", strerror(errno));
      }

      int one = 1;
      if (setsockopt(m_sd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one)) < 0) {
        if (ReactorFactory::verbose->get())
          HT_ERRORF("setsockopt(SO_KEEPALIVE) failure: %s", strerror(errno));
      }

      if (getsockname(m_sd, (struct sockaddr *)&m_local_addr, &name_len) < 0) {
        if (ReactorFactory::verbose->get())
          HT_INFOF("getsockname(%d) failed - %s", m_sd, strerror(errno));
	break;
      }
      
      //HT_INFO("Connection established.");
      m_connected = true;
      deliver_conn_estab_event = true;
    }

    //HT_INFO("about to flush send queue");
    if ((error = flush_send_queue()) != Error::OK) {
      HT_DEBUG("error flushing send queue");
      if (m_error == Error::OK)
        m_error = error;
      return true;
    }
    //HT_INFO("about to remove poll interest");
    if (m_send_queue.empty()) {
      if ((error = remove_poll_interest(PollEvent::WRITE)) != Error::OK) {
        if (m_error == Error::OK)
          m_error = error;
        return true;
      }
    }

    rval = false;
    break;
  }

  if (deliver_conn_estab_event) {
    if (ReactorFactory::proxy_master) {
      if ((error = ReactorRunner::handler_map->propagate_proxy_map(this))
          != Error::OK) {
        if (ReactorFactory::verbose->get())
          HT_ERRORF("Problem sending proxy map to %s - %s",
                    m_addr.format().c_str(), Error::get_text(error));
        return true;
      }
    }
    EventPtr event = make_shared<Event>(Event::CONNECTION_ESTABLISHED, m_addr,
                                        m_proxy, Error::OK);
    deliver_event(event);
  }

  return rval;
}


int
IOHandlerData::send_message(CommBufPtr &cbp, uint32_t timeout_ms,
                            DispatchHandler *disp_handler) {
  lock_guard<mutex> lock(m_mutex);
  bool initially_empty = m_send_queue.empty() ? true : false;
  int error = Error::OK;

  if (m_decomissioned)
    return Error::COMM_NOT_CONNECTED;

  // If request, Add message ID to request cache
  if (cbp->header.id != 0 && disp_handler != 0
      && cbp->header.flags & CommHeader::FLAGS_BIT_REQUEST) {
    auto expire_time = ClockT::now() + chrono::milliseconds(timeout_ms);
    m_reactor->add_request(cbp->header.id, this, disp_handler, expire_time);
  }

  //HT_INFOF("About to send message of size %d", cbp->header.total_len);

  m_send_queue.push_back(cbp);

  if (m_connected) {
    if ((error = flush_send_queue()) != Error::OK) {
      if (ReactorFactory::verbose->get())
        HT_WARNF("Problem flushing send queue - %s", Error::get_text(error));
      ReactorRunner::handler_map->decomission_handler(this);
      if (m_error == Error::OK)
        m_error = error;
      return error;
    }
  }

  if (initially_empty && !m_send_queue.empty()) {
    error = add_poll_interest(PollEvent::WRITE);
    if (error && ReactorFactory::verbose->get())
      HT_ERRORF("Adding Write interest failed; error=%u", (unsigned)error);
  }
  else if (!initially_empty && m_send_queue.empty()) {
    error = remove_poll_interest(PollEvent::WRITE);
    if (error && ReactorFactory::verbose->get())
      HT_INFOF("Removing Write interest failed; error=%u", (unsigned)error);
  }

  // Set m_error if not already set
  if (error != Error::OK && m_error == Error::OK)
    m_error = error;

  return error;
}


#if defined(__linux__)

int IOHandlerData::flush_send_queue() {
  ssize_t nwritten, towrite, remaining;
  struct iovec vec[2];
  int count;
  int error = 0;

  while (!m_send_queue.empty()) {

    CommBufPtr &cbp = m_send_queue.front();

    count = 0;
    towrite = 0;
    remaining = cbp->data.size - (cbp->data_ptr - cbp->data.base);
    if (remaining > 0) {
      vec[0].iov_base = (void *)cbp->data_ptr;
      vec[0].iov_len = remaining;
      towrite = remaining;
      ++count;
    }
    if (cbp->ext.base != 0) {
      remaining = cbp->ext.size - (cbp->ext_ptr - cbp->ext.base);
      if (remaining > 0) {
        vec[count].iov_base = (void *)cbp->ext_ptr;
        vec[count].iov_len = remaining;
        towrite += remaining;
        ++count;
      }
    }

    nwritten = et_socket_writev(m_sd, vec, count, &error);
    if (nwritten == (ssize_t)-1) {
      if (error == EAGAIN)
        return Error::OK;
      if (ReactorFactory::verbose->get())
        HT_WARNF("FileUtils::writev(%d, len=%d) failed : %s", m_sd, (int)towrite,
                 strerror(errno));
      return Error::COMM_BROKEN_CONNECTION;
    }
    else if (nwritten < towrite) {
      if (nwritten == 0) {
        if (error == EAGAIN)
          break;
        if (error) {
          if (ReactorFactory::verbose->get())
            HT_WARNF("FileUtils::writev(%d, len=%d) failed : %s", m_sd,
                     (int)towrite, strerror(error));
          return Error::COMM_BROKEN_CONNECTION;
        }
        continue;
      }
      remaining = cbp->data.size - (cbp->data_ptr - cbp->data.base);
      if (remaining > 0) {
        if (nwritten < remaining) {
          cbp->data_ptr += nwritten;
          if (error == EAGAIN)
            break;
          error = 0;
          continue;
        }
        else {
          nwritten -= remaining;
          cbp->data_ptr += remaining;
        }
      }
      if (cbp->ext.base != 0) {
        cbp->ext_ptr += nwritten;
        if (error == EAGAIN)
          break;
        error = 0;
        continue;
      }
    }

    // buffer written successfully, now remove from queue (destroys buffer)
    m_send_queue.pop_front();
  }

  return Error::OK;
}

#elif defined(__APPLE__) || defined (__sun__) || defined(__FreeBSD__)

int IOHandlerData::flush_send_queue() {
  ssize_t nwritten, towrite, remaining;
  struct iovec vec[2];
  int count;

  while (!m_send_queue.empty()) {

    CommBufPtr &cbp = m_send_queue.front();

    count = 0;
    towrite = 0;
    remaining = cbp->data.size - (cbp->data_ptr - cbp->data.base);
    if (remaining > 0) {
      vec[0].iov_base = (void *)cbp->data_ptr;
      vec[0].iov_len = remaining;
      towrite = remaining;
      ++count;
    }
    if (cbp->ext.base != 0) {
      remaining = cbp->ext.size - (cbp->ext_ptr - cbp->ext.base);
      if (remaining > 0) {
        vec[count].iov_base = (void *)cbp->ext_ptr;
        vec[count].iov_len = remaining;
        towrite += remaining;
        ++count;
      }
    }

    nwritten = FileUtils::writev(m_sd, vec, count);
    if (nwritten == (ssize_t)-1) {
      if (ReactorFactory::verbose->get())
        HT_WARNF("FileUtils::writev(%d, len=%d) failed : %s", m_sd, (int)towrite,
                 strerror(errno));
      return Error::COMM_BROKEN_CONNECTION;
    }
    else if (nwritten < towrite) {
      if (nwritten == 0)
        break;
      remaining = cbp->data.size - (cbp->data_ptr - cbp->data.base);
      if (remaining > 0) {
        if (nwritten < remaining) {
          cbp->data_ptr += nwritten;
          break;
        }
        else {
          nwritten -= remaining;
          cbp->data_ptr += remaining;
        }
      }
      if (cbp->ext.base != 0) {
        cbp->ext_ptr += nwritten;
        break;
      }
    }

    // buffer written successfully, now remove from queue (destroys buffer)
    m_send_queue.pop_front();
  }

  return Error::OK;
}

#else
  ImplementMe;
#endif

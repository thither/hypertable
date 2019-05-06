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
 * Declarations for DispatchHandler.
 * This file contains type declarations for DispatchHandler, an abstract
 * base class from which are derived handlers for delivering communication
 * events to an application.
 */

#ifndef AsyncComm_DispatchHandler_h
#define AsyncComm_DispatchHandler_h

#include "Event.h"

#include <memory>

namespace Hypertable {

  /** @addtogroup AsyncComm
   *  @{
   */

  /** Abstract base class for application dispatch handlers registered with
   * AsyncComm.  Dispatch handlers are the mechanism by which an application
   * is notified of communication events.
   */
  class DispatchHandler : public std::enable_shared_from_this<DispatchHandler> {
  public:

    /** Destructor
     */
    virtual ~DispatchHandler() { return; }

    /** Callback method.  When the Comm layer needs to deliver an event to the
     * application, this method is called to do so.  The set of event types
     * include, CONNECTION_ESTABLISHED, DISCONNECT, MESSAGE, ERROR, and TIMER.
     *
     * @param event_ptr smart pointer to Event object
     */
    virtual void handle(EventPtr &event_ptr) { 
      // Not a pure method,
      // instead of wait and keep a handler method for outstanding events in parent
      // do a discard
      return;
    }

  };

  /// Smart pointer to DispatchHandler
  typedef std::shared_ptr<DispatchHandler> DispatchHandlerPtr;
  /** @}*/
}

#endif // AsyncComm_DispatchHandler_h

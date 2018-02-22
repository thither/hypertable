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

package org.hypertable.FsBroker.Lib;

import java.net.ProtocolException;
import java.util.logging.Logger;
import org.hypertable.AsyncComm.ApplicationHandler;
import org.hypertable.AsyncComm.ApplicationQueue;
import org.hypertable.AsyncComm.Comm;
import org.hypertable.AsyncComm.Event;
import org.hypertable.AsyncComm.ResponseCallback;
import org.hypertable.AsyncComm.ReactorFactory;
import org.hypertable.Common.Error;
import org.hypertable.Common.Serialization;

public class RequestHandlerShutdown extends ApplicationHandler {

  static final Logger log = Logger.getLogger("org.hypertable");

  static final byte VERSION = 1;

  public RequestHandlerShutdown(Comm comm, ApplicationQueue appQueue,
                                Broker broker, Event event) {
    super(event);
    mComm = comm;
    mAppQueue = appQueue;
    mBroker = broker;
  }

  public void run() {
    ResponseCallback cb = new ResponseCallback(mComm, mEvent);

    try {

      if (mEvent.payload.remaining() < 2)
        throw new ProtocolException("Truncated message");

      int version = (int)mEvent.payload.get();
      if (version != VERSION)
        throw new ProtocolException("Shutdown parameters version mismatch, expected " +
                                    VERSION + ", got " + version);

      int encoding_length = Serialization.DecodeVInt32(mEvent.payload);
      int start_position = mEvent.payload.position();

      mBroker.Shutdown();

      mAppQueue.Shutdown();

      mBroker.GetOpenFileMap().RemoveAll();

      short flags = mEvent.payload.getShort();

      if ((flags & Protocol.SHUTDOWN_FLAG_IMMEDIATE) != 0) {
        log.info("Immediate shutdown.");
        System.exit(0);
      }

      cb.response_ok();

      ReactorFactory.Shutdown();
    }
    catch (Exception e) {
      int error = cb.error(Error.PROTOCOL_ERROR, e.getMessage());
      log.severe("Protocol error (SHUTDOWN) - " + e.getMessage());
      if (error != Error.OK)
        log.severe("Problem sending (SHUTDOWN) error back to client - "
                   + Error.GetText(error));
    }
    System.exit(0);
  }

  private Comm mComm;
  private ApplicationQueue mAppQueue;
  private Broker mBroker;
}
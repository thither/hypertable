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
/// Declarations for ConfigHandler.
/// This file contains declarations for ConfigHandler, a dispatch handler class
/// used to collect and publish %Hyperspace metrics.

#include <Common/Compat.h>

#include <Common/Error.h>
#include <Common/Logger.h>
#include <AsyncComm/ConfigHandler.h>

#include <Common/FileUtils.h>
#include <Common/Config.h>


namespace Hypertable { namespace Config {

ConfigHandler::ConfigHandler(PropertiesPtr &props) {
  m_props = props;

  m_chk_interval = m_props->get_ptr<gInt32t>("Hypertable.Config.OnFileChange.Reload.Interval");
  m_reload = m_props->get_ptr<gBool>("Hypertable.Config.OnFileChange.Reload");

  //HT_INFOF("ConfigHandler interval->get():%d", m_chk_interval->get());
  //HT_INFOF("ConfigHandler interval->get():%d", (int32_t)m_chk_interval->get());
  
  m_filename = m_props->get_str("config");
  m_comm = Comm::instance();
  m_last_timestamp = FileUtils::modification(m_filename);;
}

ConfigHandler::~ConfigHandler() {
}

void ConfigHandler::stop() {
  DispatchHandlerPtr handler = shared_from_this();
  m_comm->cancel_timer(handler);
}

void ConfigHandler::run() {
  int error;
  if ((error = m_comm->set_timer((int32_t)m_chk_interval->get(), shared_from_this())) 
            != Error::OK)
    HT_FATALF("Problem setting timer - %s", Error::get_text(error));
}

void ConfigHandler::handle(Hypertable::EventPtr &event) {
  int error;

  if (event->type == Hypertable::Event::TIMER) {
    
    errno = 0;
    time_t ts = FileUtils::modification(m_filename);
    HT_DEBUGF("ConfigHandler interval:%d last_ts: %ld ts: %ld", 
              (int32_t)m_chk_interval->get(), m_last_timestamp, ts);
    if(errno>0) 
      HT_WARNF("cfg file Problem '%s' - %s", m_filename.c_str(), strerror(errno));

    if(ts > m_last_timestamp) {
      HT_INFO(m_props->reload(m_filename, cmdline_hidden_desc(), false).c_str());
      HT_INFOF("ConfigHandler interval:%d last_ts: %ld ts: %ld",  
               (int32_t)m_chk_interval->get(), m_last_timestamp, ts);
      if (!m_reload->get()){
        HT_INFO("ConfigHandler Hypertable.Config.OnFileChange.Reload set to False,"
                 "stopping cfg reloads");
        return;
      }
      m_last_timestamp = ts;
    }

    if ((error = m_comm->set_timer((int32_t)m_chk_interval->get(), shared_from_this())) 
              != Error::OK)
      HT_FATALF("Problem setting timer - %s", Error::get_text(error));

  }
  else
    HT_FATALF("Unrecognized event - %s", event->to_str().c_str());

}

}}

/**
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
 * along with Hypertable. If not, see <http://www.gnu.org/licenses/>
 */
 
#include <Common/Compat.h>
#include <Common/String.h>

#include <ThriftBroker/Client.h>
#include <Hypertable/Lib/KeySpec.h>

#include <iostream>
#include <thread>
#include <errno.h>

#include <queue>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>

#include <pybind11/pybind11.h>
namespace py = pybind11;


namespace PyHelpers {


typedef std::shared_ptr<std::vector<Hypertable::ThriftGen::Cell>> CellsPtr;

struct TableData {
  CellsPtr cells = std::make_shared<std::vector<Hypertable::ThriftGen::Cell>>();
  uint32_t ts;
  int32_t buf_sz=0;
};
typedef std::shared_ptr<TableData> TableDataPtr;


uint32_t ts_ms_now(){
  return std::chrono::duration_cast<std::chrono::milliseconds>(
     std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}


class DispatchHandler: std::enable_shared_from_this<DispatchHandler>{

  public:

    DispatchHandler(Hypertable::Thrift::Transport ttp, 
                    Hypertable::String host, uint16_t port, Hypertable::String ns,
                    int32_t interval, int32_t size, uint32_t count,
                    bool debug, bool use_mutator, bool do_log) :
                    m_interval(interval), m_size(size), m_count(count), m_debug(debug),
                    m_use_mutator(use_mutator), m_do_log(do_log) {
      m_ttp = ttp;
      m_host = host;
      m_port = port;
      m_ns = ns;
    }

    void run(){
      int32_t time_to_wait = m_interval;
      std::vector<Hypertable::ThriftGen::Cell> cells;
      Hypertable::String table;
      bool run;

      do {
        run = m_run.load();
        cells = get_next_table(time_to_wait, run, table);
        if(!cells.empty()){
          if(m_use_mutator)
            commit_mutator(table, cells);
          else
            commit(table, cells);
        } else if(!run) 
          break;

        if(time_to_wait > 0 && run)
          std::this_thread::sleep_for(std::chrono::milliseconds(time_to_wait));

      } while (true);
        
      close_connection();  
      if(m_debug)
        log_error("WriteDispatcher stopped");
        
      m_stopped.store(true);
    }

    bool verify_runs(){
      if(!m_stopped.load()) 
        return true;

      m_run.store(true);
      m_stopped.store(false);

      return false;
    }
    
    void table_add_cell(const char * t_name, Hypertable::ThriftGen::Cell cell){
      std::lock_guard<std::mutex> lock(m_mutex);

      TableDataPtr table = !has_table(t_name)?add_table(t_name):m_tables->at(t_name);
      table->buf_sz += cell.key.row.length()
                     + cell.key.column_family.length()
                     + cell.key.column_qualifier.length()
                     + cell.value.length();
      table->cells->push_back(cell);
      table->ts = ts_ms_now();
       
      if(m_debug){
        std::stringstream ss;
        ss <<  "table_add_cell: " << t_name << ":" << (table->cells->back());
        log_error(ss.str());
      }
    }

    std::string get_log_msg() {
      std::lock_guard<std::mutex> lock(m_mutex_log);
      
      std::string msg;
      if(m_errors_log.size() == 0)
        return msg;
      
      msg = *(m_errors_log.front().get());
      m_errors_log.pop();
      return msg;
    }

    void shutdown(){
      m_run.store(false);
    }
    
    operator DispatchHandler*() { 
      return this;
    }

    virtual ~DispatchHandler(){
      shutdown();
    }

  private:

    bool has_table(const char * name) {
      return m_tables->count(name);
    }
    
    TableDataPtr add_table(const char * name){
      return m_tables->insert(TablePair(name, std::make_shared<TableData>())).first->second;
    }

    std::vector<Hypertable::ThriftGen::Cell> get_next_table(int32_t &time_to_wait, bool run, Hypertable::String &table){
      uint32_t ts= ts_ms_now();
      int32_t remain;
      time_to_wait = m_interval;
      TableDataPtr chk;

      CellsPtr cells = std::make_shared<std::vector<Hypertable::ThriftGen::Cell>>();

      std::lock_guard<std::mutex> lock(m_mutex);
      
      if(m_table_it == m_tables->end() || !run)
        m_table_it = m_tables->begin();
      
      for (;m_table_it != m_tables->end(); ++m_table_it) {
        if(m_table_it->second->buf_sz == 0)
          continue;
          
        chk = m_table_it->second;
        if(!run || chk->buf_sz >= m_size
                || chk->cells->size()  >= m_count
                || (int32_t)(ts-chk->ts) >= m_interval)
          {
            if(m_debug){
              std::stringstream ss;
              ss <<  "get_next_table: " << m_table_it->first 
                 << " buf_sz:"   << chk->buf_sz 
                 << " cells:"    << chk->cells->size() 
                 << " elapsed:"  << ts-chk->ts;              
              log_error(ss.str());
            }      
            cells.swap(chk->cells);
            chk->buf_sz = 0;
            chk->ts = ts;

            time_to_wait = 0;
            table = m_table_it->first;
            return *(cells.get());
          }

        remain = m_interval-(ts-chk->ts);
        if(remain < time_to_wait)
          time_to_wait = remain;
      }         
      return *(cells.get());
    }

    void commit(Hypertable::String table, std::vector<Hypertable::ThriftGen::Cell> cells){
      while (true){
        try {
          if(conn_client == nullptr) 
            set_ns_connection();
          conn_client->set_cells(conn_ns, table, cells);
          table.clear();
          return;
        
        } catch (Hypertable::ThriftGen::ClientException &e) {
          log_error(e.what());
          if(e.code == Hypertable::Error::BAD_KEY || e.code == Hypertable::Error::BAD_VALUE)
            return;
          close_connection();  

        } catch(...){
          log_error("Unknown Error, commit table: " + table);
          close_connection(); 
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
      }
    }

    void commit_mutator(Hypertable::String table, std::vector<Hypertable::ThriftGen::Cell> cells){
      while (true){
        try {
          if(conn_client == nullptr) 
            set_ns_connection();
          
          conn_client->mutator_set_cells(get_mutator(table), cells);
          table.clear();
          return;
        
        } catch (Hypertable::ThriftGen::ClientException &e) {
          log_error(e.what());
          if(e.code == Hypertable::Error::BAD_KEY || e.code == Hypertable::Error::BAD_VALUE)
            return;
          close_connection();  

        } catch(...){
          log_error("Unknown Error, commit_mutator table: " + table);
          close_connection(); 
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
      }
    }

    Hypertable::ThriftGen::Mutator get_mutator(Hypertable::String t_name){
      if(m_mutators.count(t_name) > 0)
        return m_mutators.at(t_name);

      if(m_debug){
        std::stringstream ss;
        ss << "Thrift::Client mutator_open " << m_host << ":" << m_port << " table: " << t_name;
        log_error(ss.str());
      }

      return m_mutators.insert(
        MutatorPair(t_name, conn_client->mutator_open(conn_ns, t_name, 0, 0))).first->second;
    }

    void close_mutators(){
      if(m_debug){  
        std::stringstream ss;
        ss << "Thrift::Client close_mutators " << m_host << ":" << m_port;
        log_error(ss.str());
      }

      for (Mutators::const_iterator it = m_mutators.begin(); it != m_mutators.end(); it++){
        try {
          conn_client->mutator_close((*it).second);
        } catch(...){
        }
      }
      m_mutators.clear();
    }
  
    void set_ns_connection(){
        if(m_debug){
          std::stringstream ss;
          ss << "Thrift::Client connecting " << m_host << ":" << m_port;
          log_error(ss.str());
        }

  	    conn_client = new Hypertable::Thrift::Client(m_ttp, m_host, m_port);
		    conn_ns = conn_client->namespace_open(m_ns);
    }

   void close_connection(){
        if(m_debug){
          std::stringstream ss;
          ss << "Thrift::Client disconnecting " << m_host << ":" << m_port;
          log_error(ss.str());
        }
        
        if(conn_client == nullptr)return;
        
        if(m_use_mutator)
          close_mutators();
        try {
          conn_client->namespace_close(conn_ns);
          conn_client->close();
        } catch(...){

        }
        conn_client = nullptr;
    }
    

    void log_error(std::string msg) {
      
      if(!m_do_log) {
          std::cerr << msg << std::endl;
          return;
      }
      if(msg.empty())
        return;

      {
        std::lock_guard<std::mutex> lock(m_mutex_log);

        while(m_errors_log.size()>1000) //keep up to N messages
          m_errors_log.pop();

        return m_errors_log.push(msg);
      }
    }



    std::atomic<bool> m_run=true;
    std::atomic<bool> m_stopped=false;
    std::mutex        m_mutex;
    std::mutex        m_mutex_log;

    typedef std::map<Hypertable::String,  TableDataPtr> Tables;
    typedef std::pair<Hypertable::String, TableDataPtr> TablePair;

    std::shared_ptr<Tables> m_tables = std::make_shared<Tables>();
    Tables::const_iterator m_table_it = m_tables->end();

    typedef std::map<Hypertable::String,  Hypertable::ThriftGen::Mutator> Mutators;
    typedef std::pair<Hypertable::String, Hypertable::ThriftGen::Mutator> MutatorPair;
    Mutators m_mutators;

    std::queue<std::string> m_errors_log;

    Hypertable::String  m_ns;
    Hypertable::String  m_host;
    uint16_t            m_port;
    Hypertable::Thrift::Transport   m_ttp;

    int32_t   m_interval;
    int32_t   m_size;
    uint32_t  m_count;
    bool      m_debug;
    bool      m_use_mutator;
    bool      m_do_log;
    Hypertable::Thrift::Client      *conn_client = nullptr;
    Hypertable::ThriftGen::Namespace conn_ns;
};


    
class WriteDispatcher {
  
  public:

    WriteDispatcher(Hypertable::String host, uint16_t port, Hypertable::String ns,
                    int32_t interval, int32_t size, uint32_t count, 
                    Hypertable::String ttp_n, 
                    bool debug, bool use_mutator, bool do_log) {

      m_handler = std::make_shared<DispatchHandler>(
        ttp_n.compare("zlib")==0 ? Hypertable::Thrift::Transport::ZLIB : Hypertable::Thrift::Transport::FRAMED, 
        host, port, ns, 
        interval, size, count, debug, 
        use_mutator,
        do_log
      );

      run();
    }
  
    std::string get_log_msg(){
      return m_handler->get_log_msg();
    }

    bool verify_runs(){
      if(m_handler->verify_runs())
        return true;
      
      run();
      return false;
    }

    void push_auto_ts(const char * t_name, 
              const char * row, const char * cf, const char * cq, 
              const Hypertable::String value, int flag){

      Hypertable::ThriftGen::Cell cell;
      cell.key.__set_flag((Hypertable::ThriftGen::KeyFlag::type)flag);

      cell.key.__set_row(row);
      cell.key.__set_column_family(cf);
      if(cq != NULL)      cell.key.__set_column_qualifier(cq);
        
      if(!value.empty())  cell.__set_value(value);
        
      m_handler->table_add_cell(t_name, cell);
    }

    void push(const char * t_name, 
              const char * row, const char * cf, const char * cq, 
              const Hypertable::String value, int64_t ts, int flag){

      Hypertable::ThriftGen::Cell cell;
      cell.key.__set_timestamp(ts);
      //cell.key.__set_revision(ts);
      cell.key.__set_flag((Hypertable::ThriftGen::KeyFlag::type)flag);

      cell.key.__set_row(row);
      cell.key.__set_column_family(cf);
      if(cq != NULL)      cell.key.__set_column_qualifier(cq);
        
      if(!value.empty())  cell.__set_value(value);
        
      m_handler->table_add_cell(t_name, cell);
    }

    operator WriteDispatcher*() { 
      return this;
    }

    void shutdown(){
      m_handler->shutdown();
    }

    virtual ~WriteDispatcher(){
      shutdown();
    }

  private:

    void run(){
      std::thread ( [d=m_handler]{ d->run(); } ).detach();
    }

    std::shared_ptr<DispatchHandler> m_handler;

};


} // PyHelpers namespace



PYBIND11_MODULE(write_dispatcher, m) {
  m.doc() = "Python Write Dipatcher Module for Hypertable ThriftClient";

  py::class_<PyHelpers::WriteDispatcher, std::shared_ptr<PyHelpers::WriteDispatcher>>(m, "WriteDispatcher")
    .def(py::init<char *, uint16_t, char *, int32_t, int32_t, uint32_t, char *, bool, bool, bool>(), 
          py::arg("host"),
          py::arg("port"),
          py::arg("namespace"),
 
          py::arg("interval") = (int32_t)10000,
          py::arg("size") = (int32_t)4194304, 
          py::arg("count") = (uint32_t)100000, 
          py::arg("trasport") = "framed",
          
          py::arg("debug") = false,
          py::arg("use_mutator") = false,
          py::arg("do_log") = false
        )
    .def("push", &PyHelpers::WriteDispatcher::push, 
           py::arg("table"), 
           py::arg("row"),
           py::arg("column_family"),
           py::arg("column_qualifier"),
           py::arg("value"),
           py::arg("timestamp"),
           py::arg("flag")
        )
    .def("push", &PyHelpers::WriteDispatcher::push_auto_ts, 
           py::arg("table"), 
           py::arg("row"),
           py::arg("column_family"),
           py::arg("column_qualifier"),
           py::arg("value"),
           py::arg("flag")=255
        )
	  .def("verify_runs",  &PyHelpers::WriteDispatcher::verify_runs)
       
    .def("get_log_msg", [](PyHelpers::WriteDispatcher* hdlr) {
      return py::bytes(hdlr->get_log_msg());
      })
	  .def("shutdown",  &PyHelpers::WriteDispatcher::shutdown)
  ;
}
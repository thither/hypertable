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

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>

#include <pybind11/pybind11.h>
namespace py = pybind11;


namespace PyHelpers {


struct TableData {
  std::vector<Hypertable::ThriftGen::Cell> cells;
  uint32_t ts;
  int32_t buf_sz=0;
};


class WriteDispatcher {
  public:
    WriteDispatcher(Hypertable::String host, uint16_t port, Hypertable::String ns,
                    int32_t interval, int32_t size, uint32_t count, Hypertable::String ttp_n, 
                    bool debug, bool use_mutator)
                    :m_interval(interval), m_size(size), m_count(count), m_debug(debug),
                     m_use_mutator(use_mutator) {
      m_host = host;
      m_port = port;
      m_ns = ns;
		  m_ttp = ttp_n.compare("zlib")==0 ? Hypertable::Thrift::Transport::ZLIB : Hypertable::Thrift::Transport::FRAMED;

      run();
    }
  
    void verify_runs(){
      if(!m_stopped.load()) 
        return;

      m_run.store(true);
      m_stopped.store(false);

      run();
    }
    
    void run(){
      try {
          std::thread ([this] { this->dispatch(); }).detach();
      } catch (std::exception& e) {
          std::cerr << e.what();
      } catch (...) {
          std::cerr << "Error Uknown Initializing WriteDispatcher - void run()";
      }
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
        
      table_add_cell(t_name, cell);
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
        
      table_add_cell(t_name, cell);
    }
      
    void table_add_cell(const char * t_name, Hypertable::ThriftGen::Cell cell){
      std::lock_guard<std::mutex> lock(m_mutex);
     
      TableData* table = !has_table(t_name)?add_table(t_name):m_tables.at(t_name);
      table->buf_sz += cell.key.row.length()
                      +cell.key.column_family.length()
                      +cell.key.column_qualifier.length()
                      +cell.value.length();
      table->cells.push_back(cell);
      table->ts= (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
       
      if(m_debug)
        std::cout <<  t_name << ": " << (table->cells.back())  << std::endl;
    }

    void shutdown(){
      m_run.store(false);
    }
    
    operator WriteDispatcher*() { 
      return this;
    }

    virtual ~WriteDispatcher(){
      shutdown();
    }

  private:

    bool has_table(const char * name) {
      return m_tables.count(name);
    }
    TableData* add_table(const char * name){
      return m_tables.insert(TablePair(name, new TableData())).first->second;
    }

    void dispatch(){
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
        std::cout <<  "WriteDispatcher stopped" << std::endl;
        
      m_stopped.store(true);
    }

    std::vector<Hypertable::ThriftGen::Cell> get_next_table(int32_t &time_to_wait, bool run, Hypertable::String &table){
      uint32_t ts= (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

      int32_t remain;

      std::vector<Hypertable::ThriftGen::Cell> cells;
      std::lock_guard<std::mutex> lock(m_mutex);

      for (const auto &kv : m_tables) {
        
        if((!run&&kv.second->cells.size()>0) || kv.second->buf_sz >= m_size || 
          kv.second->cells.size() >= m_count || 
          (int32_t)(ts-kv.second->ts) >= m_interval
        )
          {
            cells.swap(kv.second->cells);
            time_to_wait = 0;
            table = kv.first;
            kv.second->ts=ts;
            return cells;
          }

        remain = m_interval-(ts-kv.second->ts);
        time_to_wait = remain < m_interval ? remain : m_interval;
      }            
      return cells;
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
          std::cerr << e << std::endl;
          if(e.code == Hypertable::Error::BAD_KEY || e.code == Hypertable::Error::BAD_VALUE)
            return;
          close_connection();  

        } catch(...){
          std::cerr << "Unknown Error, commit table: " << table << std::endl;
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
          std::cerr << e << std::endl;
          if(e.code == Hypertable::Error::BAD_KEY || e.code == Hypertable::Error::BAD_VALUE)
            return;
          close_connection();  

        } catch(...){
          std::cerr << "Unknown Error, commit_mutator table: " << table << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
      }
    }

    Hypertable::ThriftGen::Mutator get_mutator(Hypertable::String t_name){
      if(m_mutators.count(t_name) > 0)
        return m_mutators.at(t_name);

      if(m_debug)
        std::cout << "Thrift::Client mutator_open " << m_host << ":" << m_port << " table: "
                  << t_name << std::endl;

      return m_mutators.insert(
        MutatorPair(t_name, conn_client->mutator_open(conn_ns, t_name, 0, 0))).first->second;
    }

    void close_mutators(){
      if(m_debug)
        std::cout << "Thrift::Client close_mutators " << m_host << ":" << m_port << std::endl;
      
      for (Mutators::const_iterator it = m_mutators.begin(); it != m_mutators.end(); it++){
        try {
          conn_client->mutator_close((*it).second);
        } catch(...){
        }
      }
      m_mutators.clear();
    }
  
    void set_ns_connection(){
        if(m_debug)
          std::cout <<  "Thrift::Client connecting " << m_host << ":" << m_port << std::endl;

  	    conn_client = new Hypertable::Thrift::Client(m_ttp, m_host, m_port);
		    conn_ns = conn_client->namespace_open(m_ns);
    }

   void close_connection(){
        if(m_debug)
          std::cout <<  "Thrift::Client disconnecting " << m_host << ":" << m_port << std::endl;

        if(conn_client == nullptr)return;
        
        if(m_use_mutator)
          close_mutators();
        try {
          conn_client->namespace_close(conn_ns);
        } catch(...){

        }
        conn_client = nullptr;
    }

    std::atomic<bool> m_run=true;
    std::atomic<bool> m_stopped=false;
    std::mutex        m_mutex;

    typedef std::map<Hypertable::String,  TableData*> Tables;
    typedef std::pair<Hypertable::String, TableData*> TablePair;
    Tables m_tables;

    typedef std::map<Hypertable::String,  Hypertable::ThriftGen::Mutator> Mutators;
    typedef std::pair<Hypertable::String, Hypertable::ThriftGen::Mutator> MutatorPair;
    Mutators m_mutators;

    Hypertable::String  m_ns;
    Hypertable::String  m_host;
    uint16_t            m_port;
    Hypertable::Thrift::Transport   m_ttp;

    int32_t   m_interval;
    int32_t   m_size;
    uint32_t  m_count;
    bool      m_debug;
    bool      m_use_mutator;

    Hypertable::Thrift::Client      *conn_client = nullptr;
    Hypertable::ThriftGen::Namespace conn_ns;
};

} // PyHelpers namespace



PYBIND11_MODULE(write_dispatcher, m) {
  m.doc() = "Python Write Dipatcher Module for Hypertable ThriftClient";

  py::class_<PyHelpers::WriteDispatcher, std::shared_ptr<PyHelpers::WriteDispatcher>>(m, "WriteDispatcher")
    .def(py::init<char *, uint16_t, char *, int32_t, int32_t, uint32_t, char *, bool, bool>(), 
          py::arg("host"),
          py::arg("port"),
          py::arg("namespace"),
 
          py::arg("interval") = (int32_t)10000,
          py::arg("size") = (int32_t)4194304, 
          py::arg("count") = (uint32_t)100000, 
          py::arg("trasport") = "framed",
          
          py::arg("debug") = false,
          py::arg("use_mutator") = false
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
	  .def("shutdown",  &PyHelpers::WriteDispatcher::shutdown)
  ;
}
#include "log_record_server.hh"
#include "ip_discovery_client.hh"
#include <util/net.hh>
#include <logger.hh>

using namespace virtdb;
using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  log_record_server::log_record_server(config_client & cfg_client)
  : zmqctx_(1),
    diag_pull_socket_(zmqctx_, ZMQ_PULL),
    diag_rep_socket_(zmqctx_, ZMQ_REP),
    diag_pub_socket_(zmqctx_, ZMQ_PUB),
    pull_worker_(std::bind(&log_record_server::pull_worker_function, this)),
    rep_worker_(std::bind(&log_record_server::rep_worker_function, this)),
    log_process_queue_(1,std::bind(&log_record_server::pub_function,this,std::placeholders::_1))
  {
    // collect hosts to bind to
    zmq_socket_wrapper::host_set hosts;
    {
      // add my ips
      net::string_vector my_ips{net::get_own_ips(true)};
      hosts.insert(my_ips.begin(), my_ips.end());
      
      // add discovered endpoints too
      hosts.insert(ip_discovery_client::get_ip(cfg_client.get_endpoint_client()));
      hosts.insert("*");
    }
    
    diag_pull_socket_.batch_tcp_bind(hosts);
    diag_rep_socket_.batch_tcp_bind(hosts);
    diag_pub_socket_.batch_tcp_bind(hosts);

    // start workers before we report endpoints
    rep_worker_.start();
    pull_worker_.start();
    
    // setting up LogRecord endpoints
    {
      pb::EndpointData ep_data;
      {
        ep_data.set_name(cfg_client.get_endpoint_client().name());
        ep_data.set_svctype(pb::ServiceType::LOG_RECORD);
        
        {
          // PULL socket
          auto conn = ep_data.add_connections();
          conn->set_type(pb::ConnectionType::PUSH_PULL);
          
          for( auto const & ep : diag_pull_socket_.endpoints() )
            *(conn->add_address()) = ep;
        }
        
        {
          // PUB socket
          auto conn = ep_data.add_connections();
          conn->set_type(pb::ConnectionType::PUB_SUB);
          
          for( auto const & ep : diag_pub_socket_.endpoints() )
            *(conn->add_address()) = ep;
        }
        
        cfg_client.get_endpoint_client().register_endpoint(ep_data);
      }
    }

    // setting up GetLogs endpoints
    {
      pb::EndpointData ep_data;
      {
        ep_data.set_name(cfg_client.get_endpoint_client().name());
        ep_data.set_svctype(pb::ServiceType::GET_LOGS);
        
        {
          // REQ-REP socket
          auto conn = ep_data.add_connections();
          conn->set_type(pb::ConnectionType::REQ_REP);
          
          for( auto const & ep : diag_rep_socket_.endpoints() )
            *(conn->add_address()) = ep;
        }
  
        cfg_client.get_endpoint_client().register_endpoint(ep_data);
      }
    }
  }
  
  bool
  log_record_server::rep_worker_function()
  {
    zmq::pollitem_t poll_item{ diag_rep_socket_.get(), 0, ZMQ_POLLIN, 0 };
    if( zmq::poll(&poll_item, 1, 3000) == -1 ||
       !(poll_item.revents & ZMQ_POLLIN) )
    {
      return true;
    }
    return true;
  }
  
  void
  log_record_server::pub_function(record_sptr record)
  {
    // TODO
  }
  
  bool
  log_record_server::pull_worker_function()
  {
    zmq::pollitem_t poll_item{ diag_pull_socket_.get(), 0, ZMQ_POLLIN, 0 };
    if( zmq::poll(&poll_item, 1, 3000) == -1 ||
       !(poll_item.revents & ZMQ_POLLIN) )
    {
      return true;
    }
    
    try
    {
      zmq::message_t message;
      if( !diag_pull_socket_.get().recv(&message) )
        return true;
      
      record_sptr rec(new pb::LogRecord);
      if( !message.data() || !message.size())
      {
        LOG_ERROR("empty log message arrived");
        return true;
      }
      
      bool parsed = rec->ParseFromArray(message.data(), message.size());
      if( !parsed )
      {
        LOG_ERROR("cannot parse LogRecord message");
        return true;
      }
      
      for( int i=0; i<rec->headers_size(); ++i )
        add_header(rec->process(),rec->headers(i));
      
      for( int i=0; i<rec->symbols_size(); ++i )
        add_symbol(rec->process(), rec->symbols(i));
      
      print_record(*rec);
      
      log_process_queue_.push(std::move(rec));
    }
    catch (const std::exception & e)
    {
      std::string text{e.what()};
      LOG_ERROR("exception caught" << V_(text));
    }
    catch (...)
    {
      LOG_ERROR("unknown excpetion caught");
    }
    return true;
  }
  
  void
  log_record_server::add_header(const process_info & proc_info,
                                const log_header & hdr)
  {
    lock l(header_mtx_);
    auto proc_it = headers_.find(proc_info);
    if( proc_it == headers_.end() )
    {
      auto success = headers_.insert(std::make_pair(proc_info,header_map()));
      proc_it = success.first;
    }
    
    auto head_it = proc_it->second.find(hdr.seqno());
    if( head_it == proc_it->second.end() )
    {
      (proc_it->second)[hdr.seqno()] = header_sptr(new pb::LogHeader(hdr));
    }
  }
  
  void
  log_record_server::add_symbol(const process_info & proc_info,
                                const symbol & sym)
  {
    lock l(symbol_mtx_);
    auto proc_it = symbols_.find(proc_info);
    if( proc_it == symbols_.end() )
    {
      auto success = symbols_.insert(std::make_pair(proc_info,symbol_map()));
      proc_it = success.first;
    }
    
    auto sym_it = proc_it->second.find(sym.seqno());
    if( sym_it == proc_it->second.end() )
    {
      (proc_it->second)[sym.seqno()] = sym.value();
    }
  }
  
  void
  log_record_server::print_data(const process_info & proc_info,
                                const log_data & data,
                                const log_header & head,
                                const symbol_map & symbol_table)
  {
    // locks held when this function enters (in this order)
    // - header_mtx_;
    // - symbol_mtx_;
    
    std::ostringstream host_and_name;
    
    if( proc_info.has_hostsymbol() )
      host_and_name << " " << resolve(symbol_table, proc_info.hostsymbol());
    
    if( proc_info.has_namesymbol() )
      host_and_name << "/" << resolve(symbol_table, proc_info.namesymbol());
    
    std::cout << '[' << proc_info.pid() << ':' << data.threadid() << "]"
              << host_and_name.str()
              << " (" << level_string(head.level())
              << ") @" << resolve(symbol_table,head.filenamesymbol()) << ':'
              << head.linenumber() << " " << resolve(symbol_table,head.functionnamesymbol())
              << "() @" << data.elapsedmicrosec() << "us ";
    
    int var_idx = 0;
    
    if( head.level() == pb::LogLevel::SCOPED_TRACE &&
       data.has_endscope() &&
       data.endscope() )
    {
      std::cout << " [EXIT] ";
    }
    else
    {
      if( head.level() == pb::LogLevel::SCOPED_TRACE )
        std::cout << " [ENTER] ";
      
      for( int i=0; i<head.parts_size(); ++i )
      {
        auto part = head.parts(i);
        
        if( part.isvariable() && part.hasdata() )
        {
          std::cout << " {";
          if( part.has_partsymbol() )
            std::cout << resolve(symbol_table, part.partsymbol()) << "=";
          
          if( var_idx < data.values_size() )
            print_variable( data.values(var_idx) );
          else
            std::cout << "'?'";
          
          std::cout << '}';
          
          ++var_idx;
        }
        else if( part.hasdata() )
        {
          std::cout << " ";
          if( var_idx < data.values_size() )
            print_variable( data.values(var_idx) );
          else
            std::cout << "'?'";
          
          ++var_idx;
        }
        else if( part.has_partsymbol() )
        {
          std::cout << " " << resolve(symbol_table, part.partsymbol());
        }
      }
    }
    std::cout << "\n";
  }
  
  void
  log_record_server::print_record(const log_record & rec)
  {
    // adds locks in this order:
    //  - header_mtx_;
    //  - symbol_mtx_;
    
    for( int i=0; i<rec.data_size(); ++i )
    {
      auto data = rec.data(i);
      {
        lock head_lock(header_mtx_);
        
        auto proc_heads = headers_.find(rec.process());
        if( proc_heads == headers_.end() )
        {
          std::cout << "missing proc-header\n";
          return;
        }
        
        auto head = proc_heads->second.find(data.headerseqno());
        if( head == proc_heads->second.end())
        {
          std::cout << "missing header-seqno\n";
          return;
        }
        
        if( !head->second )
        {
          std::cout << "empty header\n";
          return;
        }
        
        {
          lock sym_lock(symbol_mtx_);
          auto proc_syms = symbols_.find(rec.process());
          if( proc_syms == symbols_.end() )
          {
            std::cout << "missing proc-symtable\n";
            return;
          }
          
          print_data( rec.process(), data, *(head->second), proc_syms->second );
        }
      }
    }
  }

  log_record_server::~log_record_server()
  {
    rep_worker_.stop();
    pull_worker_.stop();
  }
  
  // static helpers:
  const std::string &
  log_record_server::resolve(const symbol_map & smap,
                             uint32_t id)
  {
    static const std::string empty("''");
    auto it = smap.find(id);
    if( it == smap.end() )
      return empty;
    else
      return it->second;
  }
  
  const std::string &
  log_record_server::level_string(log_level level)
  {
    static std::map<pb::LogLevel, std::string> level_map{
      { pb::LogLevel::INFO,          "INFO", },
      { pb::LogLevel::ERROR,         "ERROR", },
      { pb::LogLevel::SIMPLE_TRACE,  "TRACE", },
      { pb::LogLevel::SCOPED_TRACE,  "SCOPED" },
    };
    static std::string unknown("UNKNOWN");
    auto it = level_map.find(level);
    if( it == level_map.end() )
      return unknown;
    else
      return it->second;
  }
  
  void
  log_record_server::print_variable(const val_type & var)
  {
    // locks held when this function enters (in this order)
    // - header_mtx_;
    // - symbol_mtx_;
    
    switch( var.type() )
    {
        // TODO : handle array parameters ...
      case pb::Kind::BOOL:   std::cout << (var.boolvalue(0)?"true":"false"); break;
      case pb::Kind::FLOAT:  std::cout << var.floatvalue(0); break;
      case pb::Kind::DOUBLE: std::cout << var.doublevalue(0); break;
      case pb::Kind::STRING: std::cout << var.stringvalue(0); break;
      case pb::Kind::INT32:  std::cout << var.int32value(0); break;
      case pb::Kind::UINT32: std::cout << var.uint32value(0); break;
      case pb::Kind::INT64:  std::cout << var.int64value(0); break;
      case pb::Kind::UINT64: std::cout << var.uint64value(0); break;
      default:               std::cout << "'unhandled-type'"; break;
    };
  }
  
}}

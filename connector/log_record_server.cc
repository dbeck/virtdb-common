#include "log_record_server.hh"
#include "ip_discovery_client.hh"
#include <util/net.hh>
#include <util/flex_alloc.hh>
#include <util/relative_time.hh>
#include <logger.hh>
#include <sstream>
#include <iomanip>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  log_record_server::log_record_server(server_context::sptr ctx,
                                       config_client & cfg_client)
  : pull_base_type(ctx,
                   cfg_client,
                   std::bind(&log_record_server::pull_handler,
                             this,
                             std::placeholders::_1),
                   pb::ServiceType::LOG_RECORD),
    pub_base_type(ctx,
                  cfg_client,
                  pb::ServiceType::LOG_RECORD),
    log_process_queue_(1,
                       std::bind(&log_record_server::process_function,
                                 this,
                                 std::placeholders::_1))
  {
    // setting up LogRecord endpoints
    {
      pb::EndpointData ep_data;
      {
        ep_data.set_name(cfg_client.get_endpoint_client().name());
        ep_data.set_svctype(pb::ServiceType::LOG_RECORD);
        ep_data.set_cmd(pb::EndpointData::ADD);
        ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
        ep_data.add_connections()->MergeFrom(pull_base_type::conn());
        ep_data.add_connections()->MergeFrom(pub_base_type::conn());        
        cfg_client.get_endpoint_client().register_endpoint(ep_data);
        ctx->add_endpoint(ep_data);
      }
    }
  }
  
  void
  log_record_server::process_function(record_sptr record)
  {
    if( !record ) return;
    
    auto const & rel_timer = util::relative_time::instance();
    arrived_at_usec usec_tm = rel_timer.get_usec();
    
    auto const & proc = record->process();
    
    for( int i = 0; i<record->data_size(); ++i )
    {
      auto d = record->mutable_data(i);
      d->set_receivedatmicrosec(usec_tm);
      lock log_lock(log_mtx_);
      logs_.push_back(std::make_tuple(usec_tm,proc,*d));
    }
    
    enrich_record(record);
    
    static std::map<pb::LogLevel,int> priority_map{
      { pb::LogLevel::VIRTDB_ERROR, 1000 },
      { pb::LogLevel::VIRTDB_INFO,   900 },
      { pb::LogLevel::VIRTDB_STATUS, 800 },
      { pb::LogLevel::VIRTDB_SIMPLE_TRACE, 700 },
      { pb::LogLevel::VIRTDB_SCOPED_TRACE, 600 },
    };
    
    auto get_prio = [](pb::LogLevel l) {
      int ret = 0;
      if( priority_map.count(l) > 0 )
        ret = priority_map[l];
      return ret;
    };
    
    pb::LogLevel level = pb::LogLevel::VIRTDB_SIMPLE_TRACE;
    int prio = get_prio(level);
    
    for( auto const & h : record->headers() )
    {
      int new_prio = get_prio(h.level());
      if( new_prio > prio )
      {
        prio = new_prio;
        level = h.level();
      }
    }
    
    int pub_size = record->ByteSize();
    if( pub_size > 0 )
    {
      // generate channel key for subscribers
      std::string hostname;
      std::string procname;
      uint32_t host_sym = UINT32_MAX;
      uint32_t proc_sym = UINT32_MAX;
      
      if( !proc.has_hostsymbol() )
        hostname = "?";
      else
        host_sym = proc.hostsymbol();
      
      if( !proc.has_namesymbol() )
        procname = "?";
      else
        proc_sym = proc.namesymbol();
      
      if( host_sym != UINT32_MAX || proc_sym != UINT32_MAX )
      {
        for( const auto & s : record->symbols() )
        {
          // go until we have filled both strings
          if( !hostname.empty() && !procname.empty() )
            break;
          
          if( s.seqno() == host_sym )
            hostname = s.value();
          else if( s.seqno() == proc_sym )
            procname = s.value();
        }
      }
      
      std::ostringstream os;
      os << (int)level << ' ' << procname << ' ' << hostname;
      pub_base_type::publish(os.str(), std::move(record));
    }
  }
  
  namespace
  {
    inline void
    check_add_id(const std::set<uint32_t> & source,
                 std::set<uint32_t> & dest,
                 uint32_t id)
    {
      if( !source.count(id) )
        dest.insert(id);
    }
  }
  
  void
  log_record_server::enrich_record(record_sptr record)
  {
    if( !record ) return;
    
    // store symbol ids
    std::set<uint32_t> rec_symbols;
    std::set<uint32_t> need_symbols;
    
    for( auto const & s : record->symbols() )
      rec_symbols.insert(s.seqno());
    
    // store header ids
    std::set<uint32_t> rec_heads;
    std::set<uint32_t> need_heads;
    for( auto const & h : record->headers() )
    {
      rec_heads.insert(h.seqno());
      check_add_id(rec_symbols, need_symbols, h.filenamesymbol());
      check_add_id(rec_symbols, need_symbols, h.functionnamesymbol());
      check_add_id(rec_symbols, need_symbols, h.logstringsymbol());
    }

    // collect missing headers
    for( auto const & d : record->data() )
      check_add_id(rec_heads, need_heads, d.headerseqno());

    // collect missing symbols
    auto proc = record->process();
    if( proc.has_hostsymbol() )
      check_add_id(rec_symbols, need_symbols, proc.hostsymbol());
    
    if( proc.has_namesymbol() )
      check_add_id(rec_symbols, need_symbols, proc.namesymbol());
    
    // collect the actual headers
    {
      lock head_lock(header_mtx_);
      
      // find the header container for the given process
      auto proc_heads = headers_.find(proc);
      if( proc_heads != headers_.end() )
      {
        // iterate over the needed headers
        for( auto hid : need_heads )
        {
          auto head_it = proc_heads->second.find(hid);
          if( head_it != proc_heads->second.end())
          {
            auto head_data = record->add_headers();
            head_data->MergeFrom(*(head_it->second));
            
            check_add_id(rec_symbols, need_symbols, head_it->second->filenamesymbol());
            check_add_id(rec_symbols, need_symbols, head_it->second->functionnamesymbol());
            check_add_id(rec_symbols, need_symbols, head_it->second->logstringsymbol());
            
            for( auto const & p : head_it->second->parts() )
            {
              if( p.has_partsymbol() )
                check_add_id(rec_symbols, need_symbols, p.partsymbol());
            }
          }
        }
      }
    }
    
    // collect symbols too
    {
      lock symbol_lock(symbol_mtx_);
      
      // find the symbol container for the given process
      auto proc_syms = symbols_.find(proc);
      if( proc_syms != symbols_.end() )
      {
        // iterate over the needed symbols
        for( auto sid : need_symbols )
        {
          auto sym_it = proc_syms->second.find(sid);
          if( sym_it != proc_syms->second.end())
          {
            auto sym_data = record->add_symbols();
            sym_data->set_seqno(sid);
            sym_data->set_value(sym_it->second);
          }
        }
      }
    }
  }
  
  void
  log_record_server::pull_handler(record_sptr rec)
  {
    for( int i=0; i<rec->headers_size(); ++i )
      add_header(rec->process(),rec->headers(i));
    
    for( int i=0; i<rec->symbols_size(); ++i )
      add_symbol(rec->process(), rec->symbols(i));
    
    print_record(*rec);
    
    log_process_queue_.push(std::move(rec));
  }
  
  bool
  log_record_server::add_header(process_headers & destination,
                                const process_info & proc_info,
                                header_sptr hdr)
  {
    bool ret = false;
    uint32_t seqno = hdr->seqno();
    auto proc_it = destination.find(proc_info);
    if( proc_it == destination.end() )
    {
      auto success = destination.insert(std::make_pair(proc_info,header_map()));
      proc_it = success.first;
    }
    
    auto head_it = proc_it->second.find(seqno);
    if( head_it == proc_it->second.end() )
    {
      ret = true;
      (proc_it->second)[seqno] = std::move(hdr);
    }
    return ret;
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
    
    double elapsed_ms = data.elapsedmicrosec() / 1000.0;
    
    std::cerr << '[' << proc_info.pid() << ':' << data.threadid() << "]"
              << host_and_name.str()
              << " (" << level_string(head.level())
              << ") @" << resolve(symbol_table,head.filenamesymbol()) << ':'
              << head.linenumber() << " " << resolve(symbol_table,head.functionnamesymbol())
              << "() @" << std::setprecision(std::numeric_limits<double>::digits10 + 1)
              << elapsed_ms << "ms ";
    
    int var_idx = 0;
    
    if( head.level() == pb::LogLevel::VIRTDB_SCOPED_TRACE &&
       data.has_endscope() &&
       data.endscope() )
    {
      std::cerr << " [EXIT] ";
    }
    else
    {
      if( head.level() == pb::LogLevel::VIRTDB_SCOPED_TRACE )
        std::cerr << " [ENTER] ";
      
      for( int i=0; i<head.parts_size(); ++i )
      {
        auto part = head.parts(i);
        
        if( part.isvariable() && part.hasdata() )
        {
          std::cerr << " {";
          if( part.has_partsymbol() )
            std::cerr << resolve(symbol_table, part.partsymbol()) << "=";
          
          if( var_idx < data.values_size() )
            print_variable( data.values(var_idx) );
          else
            std::cerr << "'?'";
          
          std::cerr << '}';
          
          ++var_idx;
        }
        else if( part.hasdata() )
        {
          std::cerr << " ";
          if( var_idx < data.values_size() )
            print_variable( data.values(var_idx) );
          else
            std::cerr << "'?'";
          
          ++var_idx;
        }
        else if( part.has_partsymbol() )
        {
          std::cerr << " " << resolve(symbol_table, part.partsymbol());
        }
      }
    }
    std::cerr << "\n";
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
          std::cerr << "missing proc-header\n";
          return;
        }
        
        auto head = proc_heads->second.find(data.headerseqno());
        if( head == proc_heads->second.end())
        {
          std::cerr << "missing header-seqno\n";
          return;
        }
        
        if( !head->second )
        {
          std::cerr << "empty header\n";
          return;
        }
        
        {
          lock sym_lock(symbol_mtx_);
          auto proc_syms = symbols_.find(rec.process());
          if( proc_syms == symbols_.end() )
          {
            std::cerr << "missing proc-symtable\n";
            return;
          }
          
          print_data( rec.process(), data, *(head->second), proc_syms->second );
        }
      }
    }
  }
  
  void
  log_record_server::cleanup_older_than(uint64_t ms)
  {
    auto const & rel_timer = util::relative_time::instance();
    uint64_t usec_tm = rel_timer.get_usec();
    uint64_t start_tm = 0;
    if( ms > usec_tm )
      return;
    else
      start_tm = usec_tm - ms;

    {
      lock log_lock(log_mtx_);
      while( !logs_.empty() )
      {
        const auto & tuple_ref = logs_.front();
        if( std::get<0>(tuple_ref) < start_tm )
          logs_.pop_front();
        else
          break;
      }
    }
  }
  
  size_t
  log_record_server::cached_log_count()
  {
    lock log_lock(log_mtx_);
    return logs_.size();
  }

  log_record_server::~log_record_server()
  {
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
      { pb::LogLevel::VIRTDB_INFO,          "INFO", },
      { pb::LogLevel::VIRTDB_ERROR,         "ERROR", },
      { pb::LogLevel::VIRTDB_SIMPLE_TRACE,  "TRACE", },
      { pb::LogLevel::VIRTDB_SCOPED_TRACE,  "SCOPED" },
      { pb::LogLevel::VIRTDB_STATUS,        "STATUS", },
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
      case pb::Kind::BOOL:   std::cerr << (var.boolvalue(0)?"true":"false"); break;
      case pb::Kind::FLOAT:  std::cerr << var.floatvalue(0); break;
      case pb::Kind::DOUBLE: std::cerr << var.doublevalue(0); break;
      case pb::Kind::STRING: std::cerr << var.stringvalue(0); break;
      case pb::Kind::INT32:  std::cerr << var.int32value(0); break;
      case pb::Kind::UINT32: std::cerr << var.uint32value(0); break;
      case pb::Kind::INT64:  std::cerr << var.int64value(0); break;
      case pb::Kind::UINT64: std::cerr << var.uint64value(0); break;
      default:               std::cerr << "'unhandled-type'"; break;
    };
  }
  
}}

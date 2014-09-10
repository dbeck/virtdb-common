
#include "logger.hh"
#include "util/active_queue.hh"
#include "util/constants.hh"
#include <iostream>
#include <sstream>

using namespace virtdb::interface;
using namespace std::placeholders;

namespace virtdb { namespace logger {

  struct log_sink::queue_impl
  {
    typedef util::active_queue<pb_logrec_sptr,util::DEFAULT_TIMEOUT_MS>
                                                      queue;
    typedef std::unique_ptr<queue>                    queue_uptr;
    queue_uptr                                        zmq_queue_;
    queue_uptr                                        print_queue_;

    queue_impl(log_sink * sink)
    : zmq_queue_(new queue(1,std::bind(&log_sink::handle_record,sink,_1))),
      print_queue_(new queue(1,std::bind(&log_sink::print_record,sink,_1)))
    {
    }
  };
  
  log_sink::log_sink_wptr log_sink::global_sink_;
  
  namespace
  {
    const std::string &
    level_string(pb::LogLevel level)
    {
      static std::map<pb::LogLevel, std::string> level_map{
        { pb::LogLevel::VIRTDB_INFO,          "INFO", },
        { pb::LogLevel::VIRTDB_ERROR,         "ERROR", },
        { pb::LogLevel::VIRTDB_SIMPLE_TRACE,  "TRACE", },
        { pb::LogLevel::VIRTDB_SCOPED_TRACE,  "SCOPED" },
      };
      
      static std::string unknown("UNKNOWN");
      auto it = level_map.find(level);
      if( it == level_map.end() )
        return unknown;
      else
        return it->second;
    }
    
    void
    print_variable(const pb::ValueType & var)
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
    
    void
    print_data(const pb::ProcessInfo & proc_info,
               const pb::LogData & data,
               const log_record & head)
    {
      std::ostringstream host_and_name;
      
      if( proc_info.has_hostsymbol() )
        host_and_name << " " << symbol_store::get(proc_info.hostsymbol());
      
      if( proc_info.has_namesymbol() )
        host_and_name << "/" << symbol_store::get(proc_info.namesymbol());
      
      std::cout << '[' << proc_info.pid() << ':' << data.threadid() << "]"
                << host_and_name.str()
                << " (" << level_string(head.level())
                << ") @" << symbol_store::get(head.file_symbol()) << ':'
                << head.line() << " " << symbol_store::get(head.func_symbol())
                << "() @" << data.elapsedmicrosec() << "us ";
      
      int var_idx = 0;
      
      if( head.level() == pb::LogLevel::VIRTDB_SCOPED_TRACE &&
         data.has_endscope() &&
         data.endscope() )
      {
        std::cout << " [EXIT] ";
      }
      else
      {
        if( head.level() == pb::LogLevel::VIRTDB_SCOPED_TRACE )
          std::cout << " [ENTER] ";
        
        auto const & head_msg = head.get_pb_header();
        
        for( int i=0; i<head_msg.parts_size(); ++i )
        {
          auto part = head_msg.parts(i);
          
          if( part.isvariable() && part.hasdata() )
          {
            std::cout << " {";
            if( part.has_partsymbol() )
              std::cout << symbol_store::get(part.partsymbol()) << "=";
            
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
            std::cout << " " << symbol_store::get(part.partsymbol());
          }
        }
      }
      std::cout << "\n";
    }

        
    void print_rec(log_sink::pb_logrec_sptr rec)
    {
      for( int i=0; i<rec->data_size(); ++i )
      {
        auto data = rec->data(i);
        auto head = header_store::get(data.headerseqno());
        if( !head )
        {
          std::cout << "empty header\n";
          return;
        }
        print_data( rec->process(), data, *head );
      }
    }
  }
  
  void
  log_sink::print_record(log_sink::pb_logrec_sptr rec)
  {
    // TODO : make this nicer...
    try
    {
      // if the message came here instead of the right 0MQ channel we don't
      // treat stuff as cached
      symbol_store::max_id_sent(0);
      header_store::reset_all();
      
      // print it
      print_rec(rec);
    }
    catch (...)
    {
      // don't ever allow any kind of exception to leave this section
      // otherwise we wouldn't be able to log in destructors
    }
  }
  
  void
  log_sink::handle_record(log_sink::pb_logrec_sptr rec)
  {
    // TODO : do something more elegant here ...
    //  - the bad assumption here is that this active_queue has
    //    only one worker thread, thus we can use the statically
    //    allocated memory for smaller messages (<64 kilobytes)
    //  - this may break when more worker threads will send out
    //    the log messages (if ever...)

    try
    {
      static char buffer[65536];
      if( rec && socket_is_valid() )
      {
        char * work_buffer = buffer;
      
        // if we don't fit into the static buffer allocate a new one
        // which will be freed when tmp goes out of scope
        std::unique_ptr<char []> tmp;
      
        int message_size = rec->ByteSize();
        if( message_size > (int)sizeof(buffer) )
        {
          tmp.reset(new char[message_size]);
          work_buffer = tmp.get();
        }
      
        // serializing into a byte array
        bool serialized = rec->SerializeToArray(work_buffer, message_size);

        if( serialized )
        {
          // sending out the message
          socket_->get().send(work_buffer, message_size);
        }
      }
    }
    catch( ... )
    {
      // we shouldn't ever throw an exception from this function otherwise we'll
      // end up in an endless exception loop
    }
  }
  
  log_sink::log_sink()
  : queue_impl_(new queue_impl(this))
  {
  }
  
  log_sink::log_sink(log_sink::socket_sptr s)
  : local_sink_(new log_sink),
    socket_(s)
  {
    local_sink_->socket_ = socket_;
    global_sink_         = local_sink_;
    // make sure we resend symbols and headers on reconnect
    symbol_store::max_id_sent(0);
    header_store::reset_all();
  }
  
  bool
  log_sink::socket_is_valid() const
  {
    if( !socket_ || !socket_->valid() )
      return false;
    else
      return true;
  }
  
  bool
  log_sink::send_record(log_sink::pb_logrec_sptr rec)
  {
    try
    {
      if( rec && queue_impl_ && queue_impl_->zmq_queue_ && socket_is_valid())
      {
        queue_impl_->zmq_queue_->push( std::move(rec) );
        return true;
      }
      else if( rec && queue_impl_ && queue_impl_->print_queue_ )
      {
        queue_impl_->print_queue_->push( std::move(rec) );
        return true;
      }
    }
    catch( ... )
    {
      // we shouldn't ever throw an exception from this function otherwise we'll
      // end up in an endless exception loop
    }
    return false;
  }
  
  log_sink::log_sink_sptr
  log_sink::get_sptr()
  {
    return global_sink_.lock();
  }
  
  log_sink::~log_sink() {}
}}

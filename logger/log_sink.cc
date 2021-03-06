
#include <logger.hh>
#include <util/active_queue.hh>
#include <util/constants.hh>
#include <util/timer_service.hh>
#include <iostream>
#include <sstream>
#include <iomanip>

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
    util::timer_service                               timer_;

    queue_impl(log_sink * sink)
    : zmq_queue_(new queue(1,std::bind(&log_sink::handle_record,sink,_1))),
      print_queue_(new queue(1,std::bind(&log_sink::print_record,sink,_1)))
    {
      timer_.schedule(300000, [](){
        // reset every 5 minutes
        symbol_store::max_id_sent(0);
        header_store::reset_all();
        return true;
      });
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
    print_variable(const pb::ValueType & var)
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
      
      double elapsed_ms = data.elapsedmicrosec() / 1000.0;
      
      std::cerr << '[' << proc_info.pid() << ':' << data.threadid() << "]"
                << host_and_name.str()
                << " (" << level_string(head.level())
                << ") @" << symbol_store::get(head.file_symbol()) << ':'
                << head.line() << " " << symbol_store::get(head.func_symbol())
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
        
        auto const & head_msg = head.get_pb_header();
        
        for( int i=0; i<head_msg.parts_size(); ++i )
        {
          auto part = head_msg.parts(i);
          
          if( part.isvariable() && part.hasdata() )
          {
            std::cerr << " {";
            if( part.has_partsymbol() )
              std::cerr << symbol_store::get(part.partsymbol()) << "=";
            
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
            std::cerr << " " << symbol_store::get(part.partsymbol());
          }
        }
      }
      std::cerr << "\n";
    }

        
    void print_rec(log_sink::pb_logrec_sptr rec)
    {
      for( int i=0; i<rec->data_size(); ++i )
      {
        auto data = rec->data(i);
        auto head = header_store::get(data.headerseqno());
        if( !head )
        {
          std::cerr << "empty header\n";
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
          if( !socket_->send(work_buffer, message_size) )
          {
            std::cerr << "failed to send message" << __FILE__ << ':' << __LINE__ << "\n";
          }
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
        queue_impl_->zmq_queue_->push( rec );
        return true;
      }
      else if( rec && queue_impl_ && queue_impl_->print_queue_ )
      {
        queue_impl_->print_queue_->push( rec );
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

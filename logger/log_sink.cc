
#include "logger.hh"
#include "util/active_queue.hh"

using namespace std::placeholders;

namespace virtdb { namespace logger {

  struct log_sink::queue_impl
  {
    typedef util::active_queue<pb_logrec_sptr>   queue;
    typedef std::unique_ptr<queue>               queue_uptr;
    queue_uptr                                   queue_;

    queue_impl(log_sink * sink)
    : queue_(new queue(1,std::bind(&log_sink::handle_record,sink,_1)))
    {
    }
  };
  
  log_sink::log_sink_wptr log_sink::global_sink_;
  
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
        if( message_size > sizeof(buffer) )
        {
          tmp.reset(new char[message_size]);
          work_buffer = tmp.get();
        }
      
        // serializing into a byte array
        bool serialized = rec->SerializeToArray(work_buffer, message_size);

        if( serialized )
        {
          // sending out the message
          socket_->send(work_buffer, message_size);
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
  }
  
  bool
  log_sink::socket_is_valid() const
  {
    if( !socket_ || !socket_->connected() )
      return false;
    else
      return true;
  }
  
  bool
  log_sink::send_record(log_sink::pb_logrec_sptr rec)
  {
    try
    {
      if( rec && queue_impl_ && queue_impl_->queue_ && socket_is_valid())
      {
        queue_impl_->queue_->push( std::move(rec) );
        return true;
      }
      else
      {
        return false;
      }
    }
    catch( ... )
    {
      // we shouldn't ever throw an exception from this function otherwise we'll
      // end up in an endless exception loop
    }
  }
  
  log_sink::log_sink_sptr
  log_sink::get_sptr()
  {
    return global_sink_.lock();
  }
  
  log_sink::~log_sink() {}
}}

#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "feeder.hh"

namespace virtdb { namespace engine {

  feeder::feeder(collector::sptr cll)
  : collector_(cll),
    act_block_{-1}
  {
  }
  
  feeder::~feeder() {}
  
  feeder::vtr::status
  feeder::next_block()
  {
    // check if we are at the end of stream, plus gather collector stats
    auto last            = collector_->last_block_id();
    auto n_received      = collector_->n_received();
    auto n_done          = collector_->n_done();
    auto n_queued        = collector_->n_queued();
    auto n_columns       = collector_->n_columns();
    auto max_block       = collector_->max_block_id();
    auto blocks_needed   = n_columns*(max_block+1);
    auto n_proc_stared   = collector_->n_process_started();
    auto n_proc_done     = collector_->n_done();
    auto n_proc_succeed  = collector_->n_process_succeed();
    
    if( last != -1 && act_block_ == last )
    {
      // no more data to be read
      return vtr::end_of_stream_;
    }
    
    collector_->process(act_block_+1, 10, false);
    
    // will need to wait for the next block
    for( int i=0; i<10; ++i )
    {
      bool got_reader = collector_->get(act_block_+1,
                                        readers_,
                                        30000);
      
      n_proc_stared   = collector_->n_process_started();
      n_proc_done     = collector_->n_done();
      n_proc_succeed  = collector_->n_process_succeed();

      if( got_reader )
      {
        ++act_block_;
        bool scheduled_b1 = false;
        bool scheduled_b2 = false;
        bool erased_prev = false;
        size_t wait_len  = 1000;
        
        // schedule the next process to give a chance the next block
        // being ready when needed
        if( act_block_ != last )
        {
          auto to_schedule = act_block_+1;
          if(collector_->max_block_id() >= to_schedule )
            wait_len = 10;
          
          auto background_process = [this,to_schedule, wait_len]() {
            collector_->process(to_schedule, wait_len, true);
            return false;
          };
          timer_svc_.schedule(1,  background_process);
          scheduled_b1 = true;
        }
        
        // same thing for the one after, if any available there
        if( act_block_+1 != last &&
            act_block_+2 <= collector_->max_block_id() )
        {
          size_t to_schedule = act_block_+2;
          auto background_process = [this,to_schedule]() {
            collector_->process(to_schedule, 10, true);
            return false;
          };
          timer_svc_.schedule(10,  background_process);
          scheduled_b2 = true;
        }
        
        // we allow 2 old block to stay in memory so give time
        // to the user to use our buffer
        if( act_block_ > 2 )
        {
          collector_->erase(act_block_-3);
          erased_prev = true;
        }
        
        LOG_TRACE("stream" <<
                  V_(last) <<
                  V_(act_block_) <<
                  V_(max_block) <<
                  V_(n_columns) <<
                  V_(n_queued) <<
                  V_(n_done) <<
                  V_(n_received) <<
                  V_(blocks_needed) <<
                  V_(n_proc_stared) <<
                  V_(n_proc_done) <<
                  V_(n_proc_succeed) << "---" <<
                  V_(scheduled_b1) <<
                  V_(scheduled_b2) <<
                  V_(erased_prev) <<
                  V_(wait_len));
        
        return vtr::ok_;
      }
      else
      {
        collector_->process(act_block_+1, 30000, true);
        
        n_proc_stared   = collector_->n_process_started();
        n_proc_done     = collector_->n_done();
        n_proc_succeed  = collector_->n_process_succeed();
      }
      
      LOG_TRACE("reader array is not yet ready" <<
                V_(last) <<
                V_(act_block_) <<
                V_(max_block) <<
                V_(n_columns) <<
                V_(n_queued) <<
                V_(n_done) <<
                V_(n_received) <<
                V_(blocks_needed) <<
                V_(n_proc_stared) <<
                V_(n_proc_done) <<
                V_(n_proc_succeed) << "---" );

      // only ask for chunks if there are missing and we processed
      // everything
      if( blocks_needed > n_received &&
          n_proc_stared == n_proc_done )
      {
        std::vector<size_t> cols;
        for( size_t i=0; i<readers_.size(); ++i )
        {
          if( readers_[i] )
          {
            cols.push_back(i);
          }
        }
        if( cols.size() )
          collector_->resend(act_block_+1, cols);
      }
    }
    
    LOG_ERROR("aborting read, couldn't get a valid reader array in 10 minutes" <<
              V_(last) <<
              V_(act_block_) <<
              V_(max_block) <<
              V_(n_columns) <<
              V_(n_queued) <<
              V_(n_done) <<
              V_(n_received) <<
              V_(blocks_needed) <<
              V_(n_proc_stared) <<
              V_(n_proc_done) <<
              V_(n_proc_succeed) << "---" );
    
    return vtr::end_of_stream_;
  }
  
  feeder::vtr::status
  feeder::get_reader(size_t col_id,
                     vtr::sptr & reader)
  {
    feeder::vtr::status ret = vtr::ok_;
    if( readers_.empty() )
    {
      // no reader so far, must initialize
      // all subsequent next_block() calls will come from
      // read* calls
      ret = next_block();
      LOG_TRACE("next block returned" << V_((int)ret));
    }
    if( ret != vtr::ok_ )
    {
      // cannot get initial reader
      return ret;
    }
    if( col_id < readers_.size() )
    {
      // return valid reader here
      reader = readers_[col_id];
      return ret;
    }
    else
    {
      LOG_ERROR("invalid argument" << V_(readers_.size()) << V_(col_id));
      ret = vtr::end_of_stream_;
      return ret;
    }
  }
  
  feeder::vtr::status
  feeder::read_string(size_t col_id,
                      char ** ptr,
                      size_t & len,
                      bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }

    // try to read the data
    ret = reader->read_string(ptr, len);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_string(ptr, len);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }

    return ret;
  }
  
  feeder::vtr::status
  feeder::read_int32(size_t col_id,
                     int32_t & v,
                     bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_int32(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_int32(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_int64(size_t col_id,
                     int64_t & v,
                     bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_int64(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_int64(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_uint32(size_t col_id,
                      uint32_t & v,
                      bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_uint32(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_uint32(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_uint64(size_t col_id,
                      uint64_t & v,
                      bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_uint64(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_uint64(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_double(size_t col_id,
                      double & v,
                      bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_double(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_double(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }

    return ret;
  }
  
  feeder::vtr::status
  feeder::read_float(size_t col_id,
                     float & v,
                     bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_float(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_float(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_bool(size_t col_id,
                    bool & v,
                    bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_bool(v);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_bool(v);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
  feeder::vtr::status
  feeder::read_bytes(size_t col_id,
                     char ** ptr,
                     size_t & len,
                     bool & null)
  {
    vtr::sptr reader;
    auto ret = get_reader(col_id, reader);
    
    // must be able to find a valid reader
    if( ret != vtr::ok_ ) { return ret; }
    
    // try to read the data
    ret = reader->read_bytes(ptr, len);
    
    // if failed, try next block
    if( ret != vtr::ok_ ) {
      ret = next_block();
      if( ret == vtr::ok_ )
        ret = get_reader(col_id, reader);
    }
    else
    {
      // succeed, let's return
      null = reader->read_null();
      return ret;
    }
    
    // no valid reader found
    if( ret != vtr::ok_ )
    {
      LOG_TRACE("no valid reader found" << V_((int)ret));
      return ret;
    }
    
    // second try for the next block
    ret = reader->read_bytes(ptr, len);
    if( ret == vtr::ok_ )
    {
      null = reader->read_null();
    }
    else
    {
      LOG_TRACE("last chance read failed" << V_((int)ret));
    }
    
    return ret;
  }
  
}}

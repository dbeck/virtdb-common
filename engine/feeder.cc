#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <engine/feeder.hh>
#include <logger.hh>

namespace virtdb { namespace engine {

  feeder::feeder(collector::sptr cll)
  : collector_(cll),
    act_block_{-1},
    next_block_timeout_ms_{30000}
  {
    std::unique_ptr<char[]> buffer;
    for( size_t i=0; i<collector_->n_columns(); ++i)
    {
      // add empty readers
      readers_.push_back(collector::reader_sptr());
    }
  }
  
  feeder::~feeder() {}
  
  const util::relative_time &
  feeder::timer() const
  {
    return timer_;
  }
  
  bool
  feeder::fetch_next()
  {
    auto last            = collector_->last_block_id();
    auto max_block       = collector_->max_block_id();
    auto n_columns       = collector_->n_columns();
    auto blocks_needed   = n_columns*(max_block+1);
    auto n_received      = collector_->n_received();
    
    util::relative_time rt;
    
    if( last != -1 && act_block_ == last )
    {
      // no more data to be read
      return false;
    }
    
    size_t timeout_ms = 50;
    size_t got_columns = 0;
    int i = 0;
    
    // resend map
    std::map<size_t, size_t> next_resend;
    for( size_t i=0; i<n_columns; ++i )
      next_resend[i] = 0;
    
    // we try for 30 seconds
    while( rt.get_msec() < next_block_timeout_ms() )
    {
      got_columns = collector_->get(act_block_+1,
                                    timeout_ms,
                                    1000,
                                    readers_);
      
      max_block = collector_->max_block_id();
      
      if( got_columns == n_columns )
      {
        ++act_block_;
        max_block = collector_->max_block_id();
        if( max_block > act_block_ )
        {
          // schedule next block's processing (decompres+PB decode)
          collector_->background_process(act_block_+1);
        }
        
        // erase old data
        if( act_block_ > 1 )
        {
          collector_->erase(act_block_-2);
        }
        return true;
      }
      
      last            = collector_->last_block_id();
      max_block       = collector_->max_block_id();
      n_columns       = collector_->n_columns();
      blocks_needed   = n_columns*(max_block+1);
      n_received      = collector_->n_received();
      
      // never resend more often than 3 seconds
      uint64_t elapsed_ms  = rt.get_msec();
      uint64_t next_ms     = elapsed_ms + 3000;
      uint64_t resend_wait = 10+(2000/(1+max_block-act_block_));

      LOG_TRACE(V_(elapsed_ms) <<
                V_(next_ms) <<
                V_(got_columns) <<
                V_(n_columns) <<
                V_(act_block_) <<
                V_(max_block) <<
                V_(n_received) <<
                V_(readers_.size()) <<
                V_(resend_wait));
      
      if( got_columns == 0 && max_block == -1 )
      {
        if( elapsed_ms > 5000 )
        {
          // nothing arrived so the query is most likely dead
          LOG_INFO("Waiting for the first block to arrive" <<
                   V_(timeout_ms) <<
                   V_(got_columns) <<
                   V_(max_block) <<
                   V_(n_columns) <<
                   V_(blocks_needed) <<
                   V_(n_received) <<
                   V_(elapsed_ms));
        }
      }
      else if( elapsed_ms > resend_wait )
      {
        // start a resend requests after 1 second
        std::vector<size_t> cols;
        for( size_t i=0; i<readers_.size(); ++i )
        {
          if( readers_[i].get() == nullptr )
          {
            // check if resending of this column is allowed
            if( next_resend[i] < elapsed_ms )
            {
              cols.push_back(i);
              next_resend[i] = next_ms;
            }
          }
        }
        
        if( cols.size() )
        {
          collector_->resend(act_block_+1, cols);
        }
      }
      timeout_ms += 50;
    }
    
    LOG_ERROR("timed out while waiting for data" <<
              V_(n_columns) <<
              V_(got_columns) <<
              V_(max_block) <<
              V_(last) <<
              V_(blocks_needed) <<
              V_(n_received) <<
              V_(i) <<
              V_(i*timeout_ms));
    
    return false;
  }
    
}}

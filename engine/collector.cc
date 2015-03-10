#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "collector.hh"
#include <logger.hh>
#include <functional>
#include <lz4/lib/lz4.h>
#include <util/relative_time.hh>

namespace virtdb { namespace engine {
  
  collector::collector(size_t n_cols,
                       resend_function resend_fun)
  : collector_{n_cols},
    queue_{4, std::bind(&collector::prrocess,this,std::placeholders::_1)},
    max_block_id_{-1},
    last_block_id_{-1},
    n_received_{0},
    n_process_started_{0},
    n_process_done_{0},
    n_process_succeed_{0},
    resend_{resend_fun}
  {
  }
  
  collector::~collector()
  {
    queue_.stop();
  }
  
  void
  collector::resend(size_t block_id,
                    const col_vec & cols)
  {
    resend_(block_id, cols);
  }
  
  size_t
  collector::n_queued() const
  {
    return queue_.n_enqueued();
    
  }
  
  size_t
  collector::n_done() const
  {
    return queue_.n_done();
  }
  
  void
  collector::prrocess(item::sptr itm)
  {
    // cannot process invalid ptr
    if( itm.get() == nullptr ) { ++n_process_done_; return; }
    
    // need a column to be processed
    if( itm->col_ == nullptr ) { ++n_process_done_; return; }
    
    // already processed
    if( itm->reader_.get() != nullptr ) { ++n_process_done_; return; }
    
    int orig_size = itm->col_->uncompressedsize();
    if( orig_size <= 0 ) { ++n_process_done_; return; }
    
    std::unique_ptr<char[]> buffer{new char[orig_size+1]};
    
    int comp_size = itm->col_->compresseddata().size();
    if( comp_size <= 0 ) { ++n_process_done_; return; }
    
    char* res_bufer = buffer.get();
    int comp_ret = LZ4_decompress_safe(itm->col_->compresseddata().c_str(),
                                       res_bufer,
                                       itm->col_->compresseddata().size(),
                                       orig_size);
    if( comp_ret <= 0 )
    {
      LOG_ERROR("failed to decompress" <<
                V_(itm->block_id_) <<
                V_(itm->col_id_) <<
                V_(itm->col_->queryid()) <<
                V_(itm->col_->name()) <<
                V_(itm->col_->endofdata()));
      
      ++n_process_done_;
      ++n_process_succeed_;
      return;
    }
    
    // add a trailing zero to terminate the buffer
    buffer[orig_size] = 0;
    
    // assign reader and update collector
    auto rdr = util::value_type_reader::construct(std::move(buffer), orig_size);
    itm->reader_ = rdr;
    // collector_.insert(itm->block_id_, itm->col_id_, itm);
    
    ++n_process_done_;
    return;
  }
  
  void
  collector::push(size_t block_id,
                  size_t col_id,
                  column_sptr data)
  {
    item::sptr i{new item};
    i->col_       = data;
    i->block_id_  = block_id;
    i->col_id_    = col_id;
    {
      lock l(mtx_);
      ++n_received_;
      if( (int64_t)data->seqno() > max_block_id_ )
      {
        max_block_id_ = data->seqno();
        if( data->endofdata() )
        {
          last_block_id_ = data->seqno();
        }
      }
    }
    collector_.insert(block_id, col_id, i);
  }
  
  void
  collector::background_process(size_t block_id)
  {
    auto row = collector_.get(block_id, 1);
    
    for( auto & i : row.first )
    {
      if( i.get() )
      {
        if( !i->reader_ )
        {
          ++n_process_started_;
          queue_.push(i);
        }
      }
    }
  }
  
  size_t
  collector::get(size_t block_id,
                 uint64_t data_timeout_ms,
                 uint64_t process_timeout_ms,
                 reader_sptr_vec & results)
  {
    LOG_SCOPED("processing" << V_(block_id) << V_(data_timeout_ms) << V_(process_timeout_ms));
    
    auto row = collector_.get(block_id, data_timeout_ms);
    
    if( row.second != collector_.n_columns() )
    {
      LOG_TRACE("timed out while waiting for data" <<
                V_(block_id)  <<
                V_(data_timeout_ms) <<
                V_(row.second) );
    }
    
    size_t n_ok      = 0;
    size_t n_pushed  = 0;
    
    results.clear();
    
    for( auto & i : row.first )
    {
      if( i.get() )
      {
        if( !i->reader_ )
        {
          ++n_process_started_;
          queue_.push(i);
          ++n_pushed;
          results.push_back(reader_sptr());
        }
        else
        {
          ++n_ok;
          results.push_back(i->reader_);
        }
      }
      else
      {
        results.push_back(reader_sptr());
      }
    }
    
    if( n_ok == n_columns() )
    {
      // we have all readers here
      return n_ok;
    }
    else if( n_pushed == 0 &&
             n_process_started() == n_process_done() )
    {
      // pushed none and all blocks processed
      return n_ok;
    }
    else
    {
      results.clear();
      queue_.wait_empty(std::chrono::milliseconds(process_timeout_ms));

      n_ok = 0;
      row = collector_.get(block_id, 1);
      
      for( auto & i : row.first )
      {
        if( i.get() && i->reader_ )
        {
          ++n_ok;
          results.push_back(i->reader_);
        }
        else
        {
          results.push_back(reader_sptr());
        }
      }
      
      return n_ok;
    }
  }
  
  void
  collector::erase(size_t block_id)
  {
    collector_.erase(block_id);
  }
  
  int64_t
  collector::max_block_id() const
  {
    int64_t ret = -1;
    {
      lock l(mtx_);
      ret = max_block_id_;
    }
    return ret;
  }
  
  int64_t
  collector::last_block_id() const
  {
    int64_t ret = -1;
    {
      lock l(mtx_);
      ret = last_block_id_;
    }
    return ret;
  }

  size_t
  collector::n_received() const
  {
    size_t ret = 0;
    {
      lock l(mtx_);
      ret = n_received_;
    }
    return ret;
  }
  
  size_t
  collector::n_columns() const
  {
    return collector_.n_columns();
  }
  
  size_t
  collector::n_process_started() const
  {
    return n_process_started_.load();
  }
  
  size_t
  collector::n_process_done() const
  {
    return n_process_done_.load();
  }

  size_t
  collector::n_process_succeed() const
  {
    return n_process_succeed_.load();
  }
  
}}

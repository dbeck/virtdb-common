#include "pool.hh"

namespace virtdb { namespace datasrc {
  
  pool::pool(size_t max_rows)
  : max_rows_{max_rows},
  allocated_{0}
  {
    on_dispose_ = [this](column::sptr && col) {
      if( col )
      {
        lock l{mtx_};
        pool_.insert(std::move(col));
        cv_.notify_all();
      }
    };
  }
  
  bool
  pool::wait_all_disposed(uint64_t timeout_ms)
  {
    {
      // initial check for block state
      lock l(mtx_);
      if( allocated_ == pool_.size() )
        return true;
    }
    
    auto wait_till = (std::chrono::steady_clock::now() +
                      std::chrono::milliseconds(timeout_ms));
    
    std::cv_status cvstat = std::cv_status::no_timeout;
    
    while( cvstat != std::cv_status::timeout )
    {
      lock l(mtx_);
      cvstat = cv_.wait_until(l, wait_till);
      if( allocated_ == pool_.size() )
        return true;
    }
    return false;
  }
  
  size_t
  pool::n_allocated()
  {
    size_t ret = 0;
    {
      lock l(mtx_);
      ret = allocated_;
    }
    return ret;
  }
  
  size_t
  pool::n_disposed()
  {
    size_t ret = 0;
    {
      lock l(mtx_);
      ret = pool_.size();
    }
    return ret;
  }
  
}}

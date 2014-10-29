#include "pool.hh"

namespace virtdb { namespace datasrc {
  
  pool::pool(size_t max_rows)
  : max_rows_{max_rows},
  allocated_{0}
  {
    on_dispose_ = [this](column::sptr col) {
      lock l{mtx_};
      pool_.push_back(col);
      cv_.notify_all();
    };
  }
  
  bool
  pool::wait_all_disposed(uint64_t timeout_ms)
  {
    // TODO
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

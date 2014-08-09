#include "log_record.hh"

namespace virtdb { namespace logger {
  
  namespace {
    std::atomic<uint32_t> g_last_id_{0};
  }
  
  uint32_t
  log_record::get_new_id()
  {
    uint32_t ret = ++(g_last_id_);
    return ret;
  }
  
}}

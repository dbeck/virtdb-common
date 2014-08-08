#include "log_record.hh"

namespace virtdb { namespace logger {
  
  namespace
  {
    // first naive implementation
    // TODO : make it fast ... later ...
    std::atomic<uint32_t>             g_last_symbol_{0};
    std::atomic<uint32_t>             g_last_id_{0};
    std::map<std::string, uint32_t>   g_symbols_;
    std::mutex                        g_mutex_;
  }

  uint32_t
  log_record::get_symbol_id(const char * str)
  {
    if( !str ) return 0;
    std::lock_guard<std::mutex> lock(g_mutex_);
    auto it = g_symbols_.find(str);
    if( it != g_symbols_.end() )
    {
      return it->second;
    }
    else
    {
      uint32_t ret = ++g_last_symbol_;
      g_symbols_[str] = ret;
      return ret;
    }
  }
  
  uint32_t
  log_record::get_new_id()
  {
    uint32_t ret = ++(g_last_id_);
    return ret;
  }
  
}}

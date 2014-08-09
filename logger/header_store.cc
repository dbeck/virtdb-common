#include "header_store.hh"
#include "log_record.hh"
#include <atomic>
#include <mutex>
#include <map>

namespace virtdb { namespace logger {
  
  // TODO : do something faster here than std::map ...
  
  namespace {
    std::atomic<uint32_t>             g_last_id_{0};
    std::map<uint32_t, bool>          g_header_sent_;
    std::map<uint32_t, log_record *>  g_ptr_map_;
    std::mutex                        g_mutex_;
  }
  
  uint32_t
  header_store::get_new_id(log_record * rec)
  {
    uint32_t ret = ++(g_last_id_);
    {
      std::lock_guard<std::mutex> lock(g_mutex_);
      g_header_sent_[ret]  = false;
      g_ptr_map_[ret]      = rec;
    }
    return ret;
  }
  
  bool
  header_store::header_sent(uint32_t id)
  {
    std::lock_guard<std::mutex> lock(g_mutex_);
    auto it = g_header_sent_.find(id);
    if( it == g_header_sent_.end() )
      return false;
    else
      return it->second;
  }
  
  void
  header_store::header_sent(uint32_t id, bool yes_no)
  {
    std::lock_guard<std::mutex> lock(g_mutex_);
    g_header_sent_[id] = yes_no;
  }

  
}}
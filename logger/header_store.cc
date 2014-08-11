#include "header_store.hh"
#include "log_record.hh"
#include <atomic>
#include <mutex>
#include <map>

namespace virtdb { namespace logger {
  
  // TODO : do something faster here than std::map ...
  
  namespace {
    std::atomic<uint32_t>                   g_last_id_{0};
    std::map<uint32_t, bool>                g_header_sent_;
    std::map<const log_record *, uint32_t>  g_ptr_map_;
    std::mutex                              g_mutex_;
  }
  
  uint32_t
  header_store::get_new_id(const log_record * rec)
  {
    uint32_t ret = ++(g_last_id_);
    {
      std::lock_guard<std::mutex> lock(g_mutex_);
      g_header_sent_[ret]  = false;
      g_ptr_map_[rec]      = ret;
    }
    return ret;
  }
  
  bool
  header_store::has_header(const log_record * rec)
  {
    std::lock_guard<std::mutex> lock(g_mutex_);
    if( g_ptr_map_.count(rec) != 0 )
      return true;
    else
      return false;
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
#include "symbol_store.hh"
#include <atomic>
#include <map>
#include <mutex>
#include <string>

namespace virtdb { namespace logger {
  
  namespace
  {
    // first naive implementation
    // TODO : make it fast ... later ...
    std::atomic<uint32_t>             g_last_symbol_{0};
    std::atomic<uint32_t>             g_last_sent_symbol_{0};
    std::map<std::string, uint32_t>   g_symbols_;
    std::map<uint32_t, std::string>   g_id_map_;
    std::mutex                        g_mutex_;
  }
  
  uint32_t
  symbol_store::get_symbol_id(const char * str)
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
      g_id_map_[ret]  = str;
      return ret;
    }
  }
  
  uint32_t
  symbol_store::max_id_sent()
  {
    return g_last_sent_symbol_;
  }
  
  void
  symbol_store::max_id_sent(uint32_t id)
  {
    g_last_sent_symbol_ = id;
  }
  
  void
  symbol_store::for_each(symbol_store::symbol_iterator fun,
                         uint32_t start_id)
  {
    std::lock_guard<std::mutex> lock(g_mutex_);
    auto it = g_id_map_.upper_bound(start_id);
    if( it == g_id_map_.end() )
      return;
    
    for( ; it != g_id_map_.end() ; ++it )
    {
      bool want_more = fun(it->second, it->first);
      if( !want_more )
        break;
    }
  }
  
  bool
  symbol_store::has_more(uint32_t id)
  {
    std::lock_guard<std::mutex> lock(g_mutex_);
    if( g_id_map_.size() == 0 )
      return false;
    auto it = g_id_map_.rbegin();
    if( it->first > id )
      return true;
    else
      return false;
  }
  
}}

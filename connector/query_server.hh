#pragma once

#include "pull_server.hh"
#include "column_location.hh"
#include <data.pb.h>

namespace virtdb { namespace connector {
  
  class query_server final : public pull_server<interface::pb::Query>
  {
  public:
    typedef pull_server<interface::pb::Query>      base_type;
    typedef base_type::item_sptr                   query_sptr;
    
    typedef std::function<void(const std::string & provider_name,
                               const column_location & location,
                               query_sptr data)> query_monitor;
  private:
   
    typedef std::map<column_location, query_monitor>   monitor_map;
    typedef std::lock_guard<std::mutex>                lock;
    
    monitor_map            qualified_monitors_;
    monitor_map            wildcard_monitors_;
    mutable std::mutex     sockets_mtx_;
    mutable std::mutex     monitors_mtx_;
    
    void handler_function(query_sptr);
    
  public:
    query_server(config_client & cfg_client);
    virtual ~query_server();
    
    void watch(const column_location & location, query_monitor);
    void remove_watches();
    void remove_watch(const column_location & location);
  };
}}

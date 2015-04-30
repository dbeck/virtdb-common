#pragma once

#include <connector/pull_server.hh>
#include <data.pb.h>

namespace virtdb { namespace connector {
  
  class query_server final : public pull_server<interface::pb::Query>
  {
  public:
    typedef pull_server<interface::pb::Query>      pull_base_type;
    typedef pull_base_type::pull_item_sptr         query_sptr;
    
    typedef std::function<void(const std::string & provider_name,
                               query_sptr data)> query_monitor;
  private:
    typedef std::map<std::string, query_monitor>           monitor_map;
    typedef std::lock_guard<std::mutex>                    lock;
    
    monitor_map            query_monitors_;
    monitor_map            table_monitors_;
    mutable std::mutex     monitors_mtx_;
    
    void handler_function(query_sptr);
    
  public:
    query_server(server_context::sptr ctx,
                 config_client & cfg_client);
    virtual ~query_server();
    
    void watch(const std::string & query_id,
               query_monitor);
    
    void watch(const std::string & query_id,
               const std::string & schema,
               const std::string & table,
               query_monitor);
    
    void remove_watches();
    
    void remove_watch(const std::string & query_id);
    
    void remove_watch(const std::string & query_id,
                      const std::string & schema,
                      const std::string & table);
  };
}}

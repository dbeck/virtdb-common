#pragma once

#include <data.pb.h>
#include <svc_config.pb.h>
#include <memory>
#include <chrono>
#include <functional>

namespace virtdb { namespace cachedb {
  
  class store final
  {
    store() = delete;
    store(const store &) = delete;
    store & operator=(const store &) = delete;
    
    struct impl;
    std::unique_ptr<impl> impl_;
    
  public:
    typedef std::chrono::system_clock::time_point   time_point;
    
    // column data
    typedef std::shared_ptr<interface::pb::Column>  column_sptr;
    typedef std::function<column_sptr(void)>        column_allocator;
    typedef std::function<bool(column_sptr)>        column_handler;

    bool add_column(const interface::pb::Query & query,
                    const time_point query_start_time,
                    const interface::pb::Column & column);
    
    bool get_columns(const interface::pb::Query & query,
                     size_t min_block_id,
                     column_allocator allocator,
                     column_handler handler);
                     
    size_t expire_columns(size_t seconds_range);
    
    // config
    typedef std::shared_ptr<interface::pb::Config>  config_sptr;
    typedef std::function<config_sptr(void)>        config_allocator;
    typedef std::function<bool(config_sptr)>        config_handler;

    bool add_config(const interface::pb::Config & cfg);
    bool get_configs(config_allocator allocator,
                     config_handler handler);
    size_t remove_configs(config_allocator allocator,
                          config_handler handler);
    size_t expire_configs(size_t seconds_range);
    size_t dedup_configs();
    
    // endpoints
    typedef std::shared_ptr<interface::pb::EndpointData>  epdata_sptr;
    typedef std::function<epdata_sptr(void)>              epdata_allocator;
    typedef std::function<bool(epdata_sptr)>              epdata_handler;

    bool add_endpoint(const interface::pb::EndpointData & epdata);
    bool get_endpoints(epdata_allocator allocator,
                       epdata_handler handler);
    size_t remove_endpoints(epdata_allocator allocator,
                            epdata_handler handler);
    size_t expire_endpoints(size_t seconds_range);
    size_t dedup_endpoints();
    
    store(size_t expiry_seconds);
    virtual ~store();
    
  };
}}

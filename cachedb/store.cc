#include "store.hh"

namespace virtdb { namespace cachedb {
  
  struct store::impl
  {
    size_t expiry_second_;
    
    impl(size_t expiry_sec) : expiry_second_{expiry_sec}
    {
    }
  };
  
  bool
  store::add_column(const interface::pb::Query & query,
                    const time_point query_start_time,
                    const interface::pb::Column & column)
  {
    return false;
  }
  
  bool
  store::get_columns(const interface::pb::Query & query,
                     size_t min_block_id,
                     column_allocator allocator,
                     column_handler handler)
  {
    return false;
  }
  
  size_t
  store::expire_columns(size_t seconds_range)
  {
    return 0;
  }
  
  bool
  store::add_config(const interface::pb::Config & cfg)
  {
    return false;
  }
  
  bool
  store::get_configs(config_allocator allocator,
                     config_handler handler)
  {
    return false;
  }
  
  size_t
  store::remove_configs(config_allocator allocator,
                        config_handler handler)
  {
    return 0;
  }
  
  size_t
  store::expire_configs(size_t seconds_range)
  {
    return 0;
  }
  
  size_t
  store::dedup_configs()
  {
    return 0;
  }
  
  bool
  store::add_endpoint(const interface::pb::EndpointData & epdata)
  {
    return false;
  }
  
  bool
  store::get_endpoints(epdata_allocator allocator,
                       epdata_handler handler)
  {
    return false;
  }
  
  size_t
  store::remove_endpoints(epdata_allocator allocator,
                          epdata_handler handler)
  {
    return 0;
  }
  
  size_t
  store::expire_endpoints(size_t seconds_range)
  {
    return 0;
  }
  
  size_t
  store::dedup_endpoints()
  {
    return 0;
  }
  
  store::store(size_t expiry_seconds) : impl_{new impl{expiry_seconds}}
  {
  }
  
  store::~store()
  {
  }
  
}}

#pragma once

#include "endpoint_client.hh"
#include "req_client.hh"
#include "sub_client.hh"
#include "service_type_map.hh"
#include <functional>

namespace virtdb { namespace connector {
  
  class config_client final :
      public req_client<interface::pb::Config, interface::pb::Config>,
      public sub_client<interface::pb::Config>
  {
    typedef req_client<interface::pb::Config, interface::pb::Config> req_base_type;
    typedef sub_client<interface::pb::Config>                        sub_base_type;
    
    endpoint_client * ep_client_;
  public:
    config_client(endpoint_client & ep_client,
                  const std::string & server_name);
    
    virtual ~config_client();
    
    endpoint_client & get_endpoint_client();
    
    void wait_valid_sub();
    void wait_valid_req();

    bool wait_valid_sub(uint64_t timeout_ms);
    bool wait_valid_req(uint64_t timeout_ms);
    
    void cleanup();
    void rethrow_error();
  };
}}

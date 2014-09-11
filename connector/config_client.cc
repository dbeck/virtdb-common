#include "config_client.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  config_client::config_client(endpoint_client & ep_client,
                               const std::string & server_name)
  : req_base_type(ep_client, server_name),
    sub_base_type(ep_client, server_name),
    ep_client_(&ep_client)
  {
  }
  
  config_client::~config_client()
  {
  }
  
  endpoint_client &
  config_client::get_endpoint_client()
  {
    return *ep_client_;
  }
  
  void
  config_client::wait_valid_sub()
  {
    sub_base_type::wait_valid();    
  }
  
  void
  config_client::wait_valid_req()
  {
    req_base_type::wait_valid();
  }
}}

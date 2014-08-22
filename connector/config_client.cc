#include "config_client.hh"

namespace virtdb { namespace connector {

  config_client::config_client(endpoint_client & ep_client)
  : ep_client_(&ep_client)
  {
  }
  
  config_client::~config_client() {}
  
  endpoint_client &
  config_client::get_endpoint_client()
  {
    return *ep_client_;
  }

}}

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  config_client::config_client(endpoint_client & ep_client)
  : ep_client_(&ep_client),
    zmqctx_(1),
    cfg_req_socket_(zmqctx_, ZMQ_REQ),
    cfg_sub_socket_(zmqctx_, ZMQ_SUB)
  {
  }
  
  config_client::~config_client() {}
  
  endpoint_client &
  config_client::get_endpoint_client()
  {
    return *ep_client_;
  }

}}

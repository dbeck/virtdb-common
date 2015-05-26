#include "monitoring_client.hh"

namespace virtdb { namespace connector {
  
  monitoring_client::monitoring_client(client_context::sptr ctx,
                                       endpoint_client & ep_clnt,
                                       const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  monitoring_client::~monitoring_client()
  {
  }
  
  void
  monitoring_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
}}

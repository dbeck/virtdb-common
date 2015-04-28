#include "cert_store_client.hh"

namespace virtdb { namespace connector {
  
  cert_store_client::cert_store_client(client_context::sptr ctx,
                                       endpoint_client & ep_clnt,
                                       const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  cert_store_client::~cert_store_client()
  {
  }
  
  void
  cert_store_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
}}

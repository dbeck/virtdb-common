#include "srcsys_credential_client.hh"

namespace virtdb { namespace connector {
  
  srcsys_credential_client::srcsys_credential_client(client_context::sptr ctx,
                                                     endpoint_client & ep_clnt,
                                                     const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  srcsys_credential_client::~srcsys_credential_client()
  {
  }
  
  void
  srcsys_credential_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
}}

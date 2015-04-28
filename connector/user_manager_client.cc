#include "user_manager_client.hh"

namespace virtdb { namespace connector {
  
  user_manager_client::user_manager_client(client_context::sptr ctx,
                                           endpoint_client & ep_clnt,
                                           const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  user_manager_client::~user_manager_client()
  {
  }
  
  void
  user_manager_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
}}

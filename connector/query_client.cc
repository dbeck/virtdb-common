#include "query_client.hh"

namespace virtdb { namespace connector {
  
  query_client::query_client(client_context::sptr ctx,
                             endpoint_client & ep_clnt,
                             const std::string & server)
  : push_base_type(ctx,
                   ep_clnt,
                   server)
  {
  }
  
  query_client::~query_client()
  {
  }
  
  void
  query_client::cleanup()
  {
    push_base_type::cleanup();
  }
  
}}

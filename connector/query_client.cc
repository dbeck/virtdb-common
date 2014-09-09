#include "query_client.hh"

namespace virtdb { namespace connector {
  
  query_client::query_client(endpoint_client & ep_clnt,
                             const std::string & server)
  : push_base_type(ep_clnt, server)
  {
  }
  
  query_client::~query_client()
  {
  }
  
}}

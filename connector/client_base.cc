#include "client_base.hh"

namespace virtdb { namespace connector {

  client_base::client_base(endpoint_client & ep_clnt,
                           const std::string & srv)
  : server_(srv)
  {
  }
  
  client_base::~client_base()
  {
  }
  
  const std::string &
  client_base::server() const
  {
    return server_;
  }
}}

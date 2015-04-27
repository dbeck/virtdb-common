#include "client_base.hh"

namespace virtdb { namespace connector {

  client_base::client_base(client_context::sptr ctx,
                           endpoint_client & ep_clnt,
                           const std::string & srv)
  : context_(ctx),
    server_(srv)
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

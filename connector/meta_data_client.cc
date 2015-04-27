#include "meta_data_client.hh"

namespace virtdb { namespace connector {

  meta_data_client::meta_data_client(client_context::sptr ctx,
                                     endpoint_client & ep_clnt,
                                     const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
    
  }
  
  meta_data_client::~meta_data_client()
  {
  }
  
  void
  meta_data_client::cleanup()
  {
    req_base_type::cleanup();
  }

}}

#include "meta_data_client.hh"
#include "service_type_map.hh"

namespace virtdb { namespace connector {

  meta_data_client::meta_data_client(endpoint_client & ep_clnt,
                                     const std::string & server)
  : req_base_type(ep_clnt, server)
  {
    
  }
  
  meta_data_client::~meta_data_client()
  {
  }

}}

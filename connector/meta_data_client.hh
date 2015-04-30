#pragma once

#include <connector/req_client.hh>
#include <meta_data.pb.h>

namespace virtdb { namespace connector {
  
  class meta_data_client final :
      public req_client<interface::pb::MetaDataRequest,
                        interface::pb::MetaData>
  {
    typedef req_client<interface::pb::MetaDataRequest,
                       interface::pb::MetaData>         req_base_type;
  public:
    meta_data_client(client_context::sptr ctx,
                     endpoint_client & ep_clnt,
                     const std::string & server);
    
    ~meta_data_client();
    
    void cleanup();
  };
}}

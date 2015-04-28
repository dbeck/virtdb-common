#pragma once

#include "req_client.hh"
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class cert_store_client :
      public req_client<interface::pb::CertStoreRequest,
                        interface::pb::CertStoreReply>
  {
    typedef req_client<interface::pb::CertStoreRequest,
                       interface::pb::CertStoreReply>     req_base_type;
  public:
    cert_store_client(client_context::sptr ctx,
                     endpoint_client & ep_clnt,
                     const std::string & server);
    
    virtual ~cert_store_client();
    virtual void cleanup();
  };
}}

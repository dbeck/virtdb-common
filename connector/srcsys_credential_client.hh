#pragma once

#include "req_client.hh"
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class srcsys_credential_client :
      public req_client<interface::pb::SourceSystemCredentialRequest,
                        interface::pb::SourceSystemCredentialReply>
  {
    typedef req_client<interface::pb::SourceSystemCredentialRequest,
                       interface::pb::SourceSystemCredentialReply>     req_base_type;
  public:
    srcsys_credential_client(client_context::sptr ctx,
                             endpoint_client & ep_clnt,
                             const std::string & server);
    
    virtual ~srcsys_credential_client();
    virtual void cleanup();
  };
}}

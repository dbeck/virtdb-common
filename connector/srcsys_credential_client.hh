#pragma once

#include <connector/req_client.hh>
#include <security.pb.h>
#include <map>

namespace virtdb { namespace connector {
  
  class srcsys_credential_client :
      public req_client<interface::pb::SourceSystemCredentialRequest,
                        interface::pb::SourceSystemCredentialReply>
  {
    typedef req_client<interface::pb::SourceSystemCredentialRequest,
                       interface::pb::SourceSystemCredentialReply>     req_base_type;
  public:
    typedef std::map<std::string, interface::pb::FieldTemplate::FieldType> name_type_map;
    
    srcsys_credential_client(client_context::sptr ctx,
                             endpoint_client & ep_clnt,
                             const std::string & server);
    
    virtual ~srcsys_credential_client();
    virtual void cleanup();
    
    // helpers
    virtual bool set_template(const std::string & src_system,
                              const name_type_map & name_types);    
  };
}}

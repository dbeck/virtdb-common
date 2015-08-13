#pragma once

#include <connector/req_client.hh>
#include <security.pb.h>
#include <map>
#include <memory>

namespace virtdb { namespace connector {
  
  class srcsys_credential_client :
      public req_client<interface::pb::SourceSystemCredentialRequest,
                        interface::pb::SourceSystemCredentialReply>
  {
    typedef req_client<interface::pb::SourceSystemCredentialRequest,
                       interface::pb::SourceSystemCredentialReply>     req_base_type;
  public:
    typedef std::shared_ptr<srcsys_credential_client> sptr;
    typedef std::map<std::string, interface::pb::FieldTemplate::FieldType> name_type_map;
    
    srcsys_credential_client(client_context::sptr ctx,
                             endpoint_client & ep_clnt,
                             const std::string & server);
    
    virtual ~srcsys_credential_client();
    virtual void cleanup();
    
    // helpers
    virtual bool set_template(const std::string & src_system,
                              const name_type_map & name_types);
    
    virtual bool get_credential(const std::string & srcsys_token,
                                const std::string & srcsys_name,
                                interface::pb::SourceSystemCredentialReply::GetCredential & cred,
                                unsigned long timeout_ms);
  };
}}

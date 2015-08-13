#pragma once

#include <connector/req_client.hh>
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class user_manager_client :
      public req_client<interface::pb::UserManagerRequest,
                        interface::pb::UserManagerReply>
  {
    typedef req_client<interface::pb::UserManagerRequest,
                       interface::pb::UserManagerReply>    req_base_type;
    
  public:
    typedef std::shared_ptr<user_manager_client> sptr;
    
    user_manager_client(client_context::sptr ctx,
                        endpoint_client & ep_clnt,
                        const std::string & server);
    
    virtual ~user_manager_client();
    virtual void cleanup();
    
    virtual bool token_is_admin(const std::string & token,
                                unsigned long timeout_ms);
    
    virtual bool get_srcsys_token(const std::string & input_token,
                                  const std::string & srcsys_name,
                                  interface::pb::UserManagerReply::GetSourceSysToken & result,
                                  unsigned long timeout_ms);
  };
}}

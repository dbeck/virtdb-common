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
    user_manager_client(client_context::sptr ctx,
                        endpoint_client & ep_clnt,
                        const std::string & server);
    
    virtual ~user_manager_client();
    virtual void cleanup();
  };
}}

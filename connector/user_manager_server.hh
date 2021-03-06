#pragma once

#include <connector/router_server.hh>
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class user_manager_server :
      public router_server<interface::pb::UserManagerRequest,
                           interface::pb::UserManagerReply>
  {
  public:
    typedef router_server<interface::pb::UserManagerRequest,
                          interface::pb::UserManagerReply>   rep_base_type;
  private:
    typedef std::lock_guard<std::mutex>  lock;
    
    void
    on_reply_fwd(const rep_base_type::req_item &,
                 rep_base_type::rep_item_sptr);
    
    void
    on_request_fwd(const rep_base_type::req_item &,
                   rep_base_type::send_rep_handler);
        
  protected:
    virtual void
    on_reply(const rep_base_type::req_item &,
             rep_base_type::rep_item_sptr);
    
    virtual void
    on_request(const rep_base_type::req_item &,
               rep_base_type::send_rep_handler);
    
  public:
    user_manager_server(server_context::sptr ctx,
                        config_client & cfg_client);
    
    virtual ~user_manager_server();
  };
}}

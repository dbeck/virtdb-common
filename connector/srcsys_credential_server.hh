#pragma once

#include "rep_server.hh"
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class srcsys_credential_server :
      public rep_server<interface::pb::SourceSystemCredentialRequest,
                        interface::pb::SourceSystemCredentialReply>
  {
  public:
    typedef rep_server<interface::pb::SourceSystemCredentialRequest,
                       interface::pb::SourceSystemCredentialReply>     rep_base_type;
  private:
    typedef std::lock_guard<std::mutex>  lock;
    
    virtual void
    on_reply(const rep_base_type::req_item &,
             rep_base_type::rep_item_sptr);
    
    virtual void
    on_request(const rep_base_type::req_item &,
               rep_base_type::send_rep_handler);
    
  public:
    srcsys_credential_server(config_client & cfg_client,
                             const std::string & name);
    virtual ~srcsys_credential_server();
    
  };
}}

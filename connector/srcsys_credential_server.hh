#pragma once

#include <connector/rep_server.hh>
#include <security.pb.h>
#include <map>
#include <string>
#include <mutex>

namespace virtdb { namespace connector {
  
  class srcsys_credential_server :
      public rep_server<interface::pb::SourceSystemCredentialRequest,
                        interface::pb::SourceSystemCredentialReply>
  {
  public:
    typedef rep_server<interface::pb::SourceSystemCredentialRequest,
                       interface::pb::SourceSystemCredentialReply>     rep_base_type;
    typedef std::shared_ptr<srcsys_credential_server>                  sptr;
    
  private:
    typedef std::shared_ptr<interface::pb::CredentialValues>                           cred_sptr;
    typedef std::shared_ptr<interface::pb::SourceSystemCredentialReply::GetTemplate>   tmpl_sptr;
    typedef std::pair<std::string, std::string>                                        name_token;
    typedef std::map<name_token, cred_sptr>                                            cred_map;
    typedef std::map<std::string, tmpl_sptr>                                           tmpl_map;
    typedef std::lock_guard<std::mutex>                                                lock;
    
    cred_map              credentials_;
    tmpl_map              templates_;
    mutable std::mutex    mtx_;
    
    void
    on_reply_fwd(const rep_base_type::req_item &,
                 rep_base_type::rep_item_sptr);
    
    void
    on_request_fwd(const rep_base_type::req_item &,
                   rep_base_type::send_rep_handler);
    
    virtual bool
    validate_token(const std::string & srcsys_token) const;
    
  protected:
    virtual void
    on_reply(const rep_base_type::req_item &,
             rep_base_type::rep_item_sptr);
    
    virtual void
    on_request(const rep_base_type::req_item &,
               rep_base_type::send_rep_handler);
    
  public:
    srcsys_credential_server(server_context::sptr ctx,
                             config_client & cfg_client);
    virtual ~srcsys_credential_server();
    
    virtual void reload_from(const std::string & path);
    virtual void save_to(const std::string & path);
    
  };
}}

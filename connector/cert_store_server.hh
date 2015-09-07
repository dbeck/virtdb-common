#pragma once

#include <connector/rep_server.hh>
#include <security.pb.h>

namespace virtdb { namespace connector {
  
  class cert_store_server :
      public rep_server<interface::pb::CertStoreRequest,
                        interface::pb::CertStoreReply>
  {
  public:
    typedef rep_server<interface::pb::CertStoreRequest,
                       interface::pb::CertStoreReply>     rep_base_type;
    typedef std::shared_ptr<cert_store_server>            sptr;
    typedef std::shared_ptr<interface::pb::Certificate>   cert_sptr;
    
  private:
    typedef std::lock_guard<std::mutex>                  lock;
    typedef std::pair<std::string, std::string>          name_key;
    typedef std::map<name_key, cert_sptr>                cert_store;
    typedef std::map<std::string, name_key>              code_key;
    
    cert_store          certs_;
    code_key            auth_codes_;
    mutable std::mutex  mtx_;
    
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
    
    virtual
    std::string generate_authcode() const;
    
    virtual
    void expire_certificate(cert_sptr);
    
    virtual
    void validate_create_request(cert_sptr);
    
    virtual
    void validate_approval_request(const std::string & auth_code,
                                   const std::string & login_token,
                                   cert_sptr);

    virtual
    void validate_delete_request(const std::string & login_token,
                                 cert_sptr);
    
    virtual
    bool allow_authcode_listing() const;
        
  public:
    cert_store_server(server_context::sptr ctx,
                      config_client & cfg_client);
    virtual ~cert_store_server();    
    
    virtual void reload_from(const std::string & path);
    virtual void save_to(const std::string & path);
  };
}}

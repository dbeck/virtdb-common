#pragma once

#include <connector/rep_server.hh>
#include <connector/pub_server.hh>
#include <connector/meta_data_store.hh>
#include <connector/user_manager_client.hh>
#include <connector/srcsys_credential_client.hh>
#include <connector/query_context.hh>
#include <meta_data.pb.h>
#include <security.pb.h>
#include <map>
#include <mutex>

namespace virtdb { namespace connector {
  
  class meta_data_server final :
      public rep_server<interface::pb::MetaDataRequest,
                        interface::pb::MetaData>,
      public pub_server<interface::pb::MetaData>
  {
  public:
    typedef std::function<void(const interface::pb::MetaDataRequest &,
                               query_context::sptr qctx)>  on_request;    
    typedef rep_server<interface::pb::MetaDataRequest,
                       interface::pb::MetaData>            rep_base_type;
    typedef pub_server<interface::pb::MetaData>            pub_base_type;
  private:
    typedef std::map<std::string, meta_data_store::sptr>   meta_store_map;
    typedef std::lock_guard<std::mutex>                    lock;
    
    // TODO: replace map<> with a better structure, more optimal for regex search
    
    server_context::sptr              ctx_;
    on_request                        on_request_;
    meta_store_map                    meta_stores_;
    user_manager_client::sptr         user_mgr_cli_;
    srcsys_credential_client::sptr    sscred_cli_;
    bool                              skip_token_check_;
    std::mutex                        stores_mtx_;
    std::mutex                        watch_mtx_;
    
    void publish_meta(const rep_base_type::req_item &,
                      rep_base_type::rep_item_sptr);
    
    void process_replies(const rep_base_type::req_item & req,
                         rep_base_type::send_rep_handler handler);
    
    bool get_srcsys_token(const std::string & input_token,
                          const std::string & service_name,
                          query_context::sptr qctx);
        
  public:
    meta_data_server(server_context::sptr ctx,
                     config_client & cfg_client,
                     user_manager_client::sptr umgr_cli,
                     srcsys_credential_client::sptr sscred_cli,
                     bool skip_token_check=false);
                     
    virtual ~meta_data_server();
    
    meta_data_store::sptr get_store(const std::string & srcsys_token);
    meta_data_store::sptr get_store(const interface::pb::UserManagerReply::GetSourceSysToken & srcsys_token);
    
    void watch_requests(on_request);
    void remove_watch();
    
    server_context::sptr ctx();
  };
  
}}

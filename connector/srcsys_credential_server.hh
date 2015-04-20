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
                           interface::pb::SourceSystemCredentialReply>      rep_base_type;
        typedef std::function<void(const rep_base_type::req_item&)>         on_request;
        typedef std::function<void(const rep_base_type::req_item&,
                                   rep_base_type::rep_item_sptr)>           on_reply;
        
    private:
        typedef std::lock_guard<std::mutex>  lock;
        
        on_request on_request_;
        on_reply   on_reply_;
      
        void process_rep(const rep_base_type::req_item&,
                         rep_base_type::rep_item_sptr);
        void process_req(const rep_base_type::req_item & req,
                         rep_base_type::send_rep_handler handler);
        
    public:
        srcsys_credential_server(config_client & cfg_client);
        virtual ~srcsys_credential_server();
        
        void watch_requests(on_request);
        void watch_replies(on_reply);
        void remove_watches();
    };
}}

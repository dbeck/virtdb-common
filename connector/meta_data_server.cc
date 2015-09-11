#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <connector/meta_data_server.hh>
#include <regex>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  meta_data_server::meta_data_server(server_context::sptr ctx,
                                     config_client & cfg_client,
                                     user_manager_client::sptr umgr_cli,
                                     srcsys_credential_client::sptr sscred_cli,
                                     bool skip_token_check)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&meta_data_server::process_replies,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&meta_data_server::publish_meta,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::META_DATA),
    pub_base_type(ctx,
                  cfg_client,
                  pb::ServiceType::META_DATA),
    ctx_{ctx},
    user_mgr_cli_{umgr_cli},
    sscred_cli_{sscred_cli},
    skip_token_check_{skip_token_check}
  {
    if( !ctx || !umgr_cli || !sscred_cli)
    {
      THROW_("invalid parameter");
    }

    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      ep_data.set_svctype(pb::ServiceType::META_DATA);
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  bool
  meta_data_server::get_srcsys_token(const std::string & input_token,
                                     const std::string & service_name,
                                     query_context::sptr qctx)
  {
    if( !user_mgr_cli_->wait_valid(100) )
    {
      LOG_ERROR("user manager client is not valid" << V_(service_name));
      return false;
    }
    
    // TODO : let's cache this later ...
    if( !user_mgr_cli_->get_srcsys_token(input_token,
                                         service_name,
                                         *(qctx->token()),
                                         util::DEFAULT_TIMEOUT_MS) )
    {
      return false;
    }
    
    return sscred_cli_->get_credential(qctx->token()->sourcesystoken(),
                                       service_name,
                                       *(qctx->credentials()),
                                       util::DEFAULT_TIMEOUT_MS);
  }
  
  void
  meta_data_server::publish_meta(const rep_base_type::req_item&,
                                 rep_base_type::rep_item_sptr)
  {
    // TODO : what do we publish here ... ????
  }
  
  void
  meta_data_server::process_replies(const rep_base_type::req_item & req,
                                    rep_base_type::send_rep_handler handler)
  {
    query_context::sptr qctx{new query_context};
    if( !skip_token_check_ )
    {
      query_context::srcsys_tok_reply_sptr tok_reply{new interface::pb::UserManagerReply::GetSourceSysToken};
      query_context::srcsys_cred_reply_stptr cred_reply{new interface::pb::SourceSystemCredentialReply::GetCredential};
      qctx->token(tok_reply);
      qctx->credentials(cred_reply);
    }

    std::string sstok{"ANY"};

    if( !skip_token_check_ )
    {    
      if( req.has_usertoken() )
      {
        std::string svc_name{rep_base_type::service_name()};
        LOG_TRACE("requesting source system token for" << V_(svc_name));
        if( !get_srcsys_token(req.usertoken(),
                              svc_name,
                              qctx) )
        {
          LOG_ERROR("cannot gather source system token for" <<
                    V_(req.name()) <<
                    V_(req.schema()) <<
                    V_(req.withfields()));
        }
      }
      else
      {
        LOG_ERROR("no user token in metadata request" <<
                  V_(req.name()) <<
                  V_(req.schema()) <<
                  V_(req.withfields()));
      }
      sstok = qctx->token()->sourcesystoken();
    }
    else if(req.has_usertoken());
    {
      sstok = req.usertoken();
    }

    {
      lock l(watch_mtx_);
      if( on_request_ )
      {
        try
        {
          on_request_(req, qctx);
        }
        catch(const std::exception & e)
        {
          std::string text{e.what()};
          LOG_ERROR("exception" << V_(text));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception in on_request function");
        }
      }
    }
    
    rep_base_type::rep_item_sptr rep;
    bool is_wildcard = false;
    
    auto store = get_store(sstok);

    if( req.has_schema() &&
        req.has_name() &&
        req.schema() == ".*" &&
        req.name() == ".*" )
    {
      rep = store->get_wildcard_data();
      is_wildcard = true;
    }
    
    if( rep && rep->tables_size() && is_wildcard && req.withfields() == false )
    {
      int64_t tables_size = 0;
      tables_size = rep->tables_size();
      LOG_INFO("returning cached wildcard metadata" << V_(tables_size));
      ctx_->increase_stat("Returing cached wildcard metadata");
    }
    else
    {
      rep.reset(new rep_item);
      store = get_store(sstok);
      rep = store->get_tables_regexp(req.schema(),
                                     req.name(),
                                     req.withfields());
    }
    
    if( !rep )
    {
      ctx_->increase_stat("Error gathering metadata");
      LOG_ERROR("couldn't gather metadata for" <<
                V_(store->size()) <<
                V_(req.name()) <<
                V_(req.schema()) <<
                V_(req.withfields()));
    }
    else if( rep->tables_size() == 0 )
    {
      ctx_->increase_stat("Metadata is empty");
      LOG_ERROR("couldn't gather metadata for" <<
                V_(store->size()) <<
                V_(req.name()) <<
                V_(req.schema()) <<
                V_(req.withfields()));
    }
    
    handler(rep, false);
  }
  
  void
  meta_data_server::watch_requests(on_request mon)
  {
    lock l(watch_mtx_);
    on_request_ = mon;
  }
  
  void
  meta_data_server::remove_watch()
  {
    lock l(watch_mtx_);
    on_request empty;
    on_request_.swap(empty);
  }
  
  meta_data_store::sptr
  meta_data_server::get_store(const std::string & srcsys_token)
  {
    meta_data_store::sptr ret;
    {
      lock l(stores_mtx_);
      if( meta_stores_.count(srcsys_token) == 0 )
      {
        ret.reset(new meta_data_store{srcsys_token});
        meta_stores_[srcsys_token] = ret;
        LOG_TRACE("created new meta data store for" << V_(srcsys_token));
      }
      ret = meta_stores_[srcsys_token];
    }
    return ret;
  }
  
  meta_data_store::sptr
  meta_data_server::get_store(const interface::pb::UserManagerReply::GetSourceSysToken & srcsys_token)
  {
    return get_store(srcsys_token.sourcesystoken());
  }

  meta_data_server::~meta_data_server() {}
  
  server_context::sptr
  meta_data_server::ctx()
  {
    return ctx_;
  }
}}


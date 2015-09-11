#include "query_server.hh"
#include <util/constants.hh>
#include <functional>

using namespace virtdb::interface;

namespace virtdb { namespace connector {
  
  query_server::query_server(server_context::sptr ctx,
                             config_client & cfg_client,
                             user_manager_client::sptr umgr_cli,
                             srcsys_credential_client::sptr sscred_cli,
                             bool skip_token_check)
  : pull_base_type(ctx,
                   cfg_client,
                   std::bind(&query_server::handler_function,
                             this,
                             std::placeholders::_1),
                   pb::ServiceType::QUERY),
    ctx_{ctx},
    umgr_cli_{umgr_cli},
    sscred_cli_{sscred_cli},
    skip_token_check_{skip_token_check}
  {
    if( !ctx || !umgr_cli || ! sscred_cli  )
    {
      THROW_("invalid parameter");
    }
    
    pb::EndpointData ep_data;
    
    ep_data.set_name(cfg_client.get_endpoint_client().name());
    ep_data.set_svctype(pb::ServiceType::QUERY);
    ep_data.set_cmd(pb::EndpointData::ADD);
    ep_data.set_validforms(util::DEFAULT_ENDPOINT_EXPIRY_MS);
    ep_data.add_connections()->MergeFrom(pull_base_type::conn());
    cfg_client.get_endpoint_client().register_endpoint(ep_data);
    ctx->add_endpoint(ep_data);
  }
  
  namespace
  {
    template <typename FUN, typename ITEM>
    void call_monitor(FUN & fun,
                      ITEM & item,
                      const std::string & src,
                      query_context::sptr qctx)
    {
      try
      {
        fun(src, item, qctx);
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown excpetion caught");
      }
    }
    
    template <typename CONTAINER, typename ITEM>
    void fire_monitors(CONTAINER & container,
                       ITEM & item,
                       const std::string & key,
                       const std::string & src,
                       query_context::sptr qctx)
    {
      try
      {
        auto it = container.find(key);
        if( it != container.end() )
          call_monitor(it->second,
                       item,
                       src,
                       qctx);
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown excpetion caught");
      }
    }
    
    std::string gen_table_key(const std::string & query_id,
                              const std::string & schema,
                              const std::string & table)
    {
      std::ostringstream os;
      os << query_id << '/' << schema << '/' << table;
      return os.str();
    }
  }
  
  void
  query_server::handler_function(query_sptr qsptr)
  {
    lock l(monitors_mtx_);
    
    if( !qsptr->has_queryid() ||
        !qsptr->has_table()   ||
       (qsptr->fields_size() == 0 && qsptr->querycontrol() != interface::pb::Query::STOP) )
    {
      ctx_->increase_stat("Invalid query");
      auto q = qsptr;
      LOG_ERROR("cannot handle invalid query" <<
                V_(q->queryid()) <<
                V_(q->table()) <<
                V_(q->schema()) <<
                V_(q->fields_size()) <<
                V_(q->limit()) <<
                V_(q->filter_size()) <<
                V_(q->has_querycontrol()) <<
                V_(q->has_usertoken()));
      return;
    }
    
    if( !skip_token_check_ )
    {
      if( !qsptr->has_usertoken() )
      {
        ctx_->increase_stat("Missing user token");
        auto q = qsptr;
        LOG_ERROR("missing user token" <<
                  V_(q->queryid()) <<
                  V_(q->table()) <<
                  V_(q->schema()) <<
                  V_(q->fields_size()) <<
                  V_(q->limit()) <<
                  V_(q->filter_size()) <<
                  V_(q->has_querycontrol()) <<
                  V_(q->has_usertoken()));
        return;
      }
    }
    
    std::string svc_name{service_name()};
    query_context::sptr qctx{new query_context};
    {
      qctx->query(qsptr);
    
      if( !skip_token_check_ )
      {
        query_context::srcsys_tok_reply_sptr tok_reply{new interface::pb::UserManagerReply::GetSourceSysToken};
        query_context::srcsys_cred_reply_stptr cred_reply{new interface::pb::SourceSystemCredentialReply::GetCredential};
            
        std::string user_token{qsptr->usertoken()};
        
        if( !umgr_cli_->get_srcsys_token(user_token,
                                         svc_name,
                                         *tok_reply,
                                         util::DEFAULT_TIMEOUT_MS) )
        {
          auto q = qsptr;
          ctx_->increase_stat("User token check failed");
          LOG_ERROR("cannot validate user token" <<
                    V_(ctx_->service_name()) <<
                    V_(svc_name) <<
                    V_(q->queryid()) <<
                    V_(q->table()) <<
                    V_(q->schema()) <<
                    V_(q->fields_size()) <<
                    V_(q->limit()) <<
                    V_(q->filter_size()) <<
                    V_(q->has_querycontrol()) <<
                    V_(q->has_usertoken()));
          return;
        }
        qctx->token(tok_reply);

        if( !sscred_cli_->get_credential(tok_reply->sourcesystoken(),
                                         svc_name,
                                         *cred_reply,
                                         util::DEFAULT_TIMEOUT_MS) )
        {
          auto q = qsptr;
          ctx_->increase_stat("Invalid source system token");
          LOG_ERROR("cannot validate source system token" <<
                    V_(ctx_->service_name()) <<
                    V_(svc_name) <<
                    V_(q->queryid()) <<
                    V_(q->table()) <<
                    V_(q->schema()) <<
                    V_(q->fields_size()) <<
                    V_(q->limit()) <<
                    V_(q->filter_size()) <<
                    V_(q->has_querycontrol()) <<
                    V_(q->has_usertoken()));
          return;
        }
      
        qctx->credentials(cred_reply);
      }
    }    
    
    ctx_->increase_stat("Valid query");
    ctx_->increase_stat("Query field count", qsptr->fields_size());
    
    if( qsptr->has_querycontrol() )
    {
      ctx_->increase_stat("Query has control command (vanilla)");
      switch ( qsptr->querycontrol() )
      {
        case interface::pb::Query::RESEND_CHUNK:
          ctx_->increase_stat("Query has RESEND_CHUNK command (vanilla)");
          break;
          
        case interface::pb::Query::RESEND_TABLE:
          ctx_->increase_stat("Query has RESEND_TABLE command (vanilla)");
          break;

        case interface::pb::Query::STOP:
          ctx_->increase_stat("Query has STOP command (vanilla)");
          break;

        default:
          ctx_->increase_stat("Query has unknown command (vanilla)");
          break;
      }
    }
    
    if( qsptr->seqnos_size() > 0 )
    {
      ctx_->increase_stat("Query has sequence numbers (vanilla)");
      ctx_->increase_stat("Query resend field count (vanilla)",
                          qsptr->seqnos_size()*
                          qsptr->fields_size());
    }
    
    // query monitors
    const std::string & n = name();
    fire_monitors(query_monitors_, qsptr, qsptr->queryid(), n, qctx);
    fire_monitors(query_monitors_, qsptr, "", n, qctx);
    
    std::string query_id = qsptr->queryid();
    std::string schema   = qsptr->schema();
    std::string table    = qsptr->table();
    
    // table monitors
    if( !table_monitors_.empty() )
    {
      fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,schema,table), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,schema,""), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,"",table), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key("",schema,table), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,"",""), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key("",schema,""), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key("","",table), n, qctx);
      fire_monitors(table_monitors_, qsptr, gen_table_key("","",""), n, qctx);
    }
  }
  
  void
  query_server::watch(const std::string & query_id,
                      query_monitor mon)
  {
    lock l(monitors_mtx_);
    query_monitors_[query_id] = mon;
  }

  void
  query_server::remove_watch(const std::string & query_id)
  {
    lock l(monitors_mtx_);
    query_monitors_.erase(query_id);
  }

  void
  query_server::watch(const std::string & query_id,
                      const std::string & schema,
                      const std::string & table,
                      query_monitor mon)
  {
    std::ostringstream os;
    os << query_id << '/' << schema << '/' << table;
    {
      lock l(monitors_mtx_);
      table_monitors_[os.str()] = mon;
    }
  }

  void
  query_server::remove_watch(const std::string & query_id,
                             const std::string & schema,
                             const std::string & table)
  {
    std::ostringstream os;
    os << query_id << '/' << schema << '/' << table;
    {
      lock l(monitors_mtx_);
      table_monitors_.erase(os.str());
    }
  }
  

  void
  query_server::remove_watches()
  {
    lock l(monitors_mtx_);
    query_monitors_.clear();
    table_monitors_.clear();
  }
  
  query_server::~query_server()
  {
  }
  
}}

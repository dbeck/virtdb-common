#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "meta_proxy.hh"
#include <svc_config.pb.h>
#include <util/constants.hh>
#include <logger.hh>

using namespace virtdb::interface;

namespace virtdb { namespace dsproxy {
  
  bool
  meta_proxy::reconnect()
  {
    std::string server;
    {
      std::unique_lock<std::mutex> l(mtx_);
      if( client_sptr_ )
        server = client_sptr_->server();
    }
    if( server.empty() )
    {
      LOG_ERROR("cannot reconnect. empty server name");
      return false;
    }
    else
    {
      return reconnect(server);
    }
  }
  
  bool
  meta_proxy::reconnect(const std::string & server)
  {
    server_ctx_->increase_stat("Connect to metadata server");
    {
      std::unique_lock<std::mutex> l(mtx_);
      client_ctx_->name(server);
      client_sptr_.reset(new connector::meta_data_client(client_ctx_,
                                                         *ep_client_,
                                                         server));
    }
    bool ret = client_sptr_->wait_valid(util::SHORT_TIMEOUT_MS);
    if( ret )
    {
      LOG_TRACE("meta client connected to:" << V_(server));
    }
    else
    {
      LOG_ERROR("meta client connection to" <<
                V_(server) << "timed out in" <<
                V_(util::SHORT_TIMEOUT_MS));
    }
    return ret;
  }
  
  void
  meta_proxy::reset_client()
  {
    on_disconnect on_disconnect_copy;
    {
      std::unique_lock<std::mutex> l(mtx_);
      on_disconnect_copy = on_disconnect_;
    }
    
    if( on_disconnect_copy )
      on_disconnect_copy();    
  }
  
  void
  meta_proxy::watch_disconnect(on_disconnect m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_disconnect_ = m;
  }
  
  meta_proxy::meta_proxy(connector::server_context::sptr sr_ctx,
                         connector::client_context::sptr cl_ctx,
                         connector::config_client & cfg_clnt,
                         connector::user_manager_client::sptr umgr_cli,
                         connector::srcsys_credential_client::sptr sscred_cli)
  : server_ctx_{sr_ctx},
    client_ctx_{cl_ctx},
    server_{sr_ctx, cfg_clnt, umgr_cli, sscred_cli},
    ep_client_{&(cfg_clnt.get_endpoint_client())}
  {
    // make sure we return the client name as the server name for the sake of proxying
    server_.meta_data_server::rep_base_type::override_service_name( [this](connector::server_context::sptr){ return client_ctx_->name(); } );
    server_.meta_data_server::pub_base_type::override_service_name( [this](connector::server_context::sptr){ return client_ctx_->name(); } );
    
    server_.watch_requests([&](const interface::pb::MetaDataRequest & req,
                               connector::query_context::sptr qctx)
    {
      LOG_SCOPED("request arrived" <<
                 V_(server_.meta_data_server::rep_base_type::name()) <<
                 V_(server_.meta_data_server::rep_base_type::service_name()) <<
                 V_(req.schema()) <<
                 V_(req.name())
                 );
      
      server_ctx_->increase_stat("Metadata proxy request");
      
      std::string sstok;
      if( qctx && qctx->token() && qctx->token()->has_sourcesystoken() )
        sstok = qctx->token()->sourcesystoken();
      
      auto store = server_.get_store(sstok);
      if( !store )
      {
        LOG_ERROR("invalid store returned for" << V_(sstok));
        return;
      }

      std::string table;
      if( req.has_name() ) table = req.name();
      
      std::string schema;
      if( req.has_schema() ) schema = req.schema();
      
      // we use our internal cache: so only forward request when needed
      if( req.withfields() )
      {
        // fast path: when we already have this table with fields
        if( store->has_fields(schema, table) )
        {
          server_ctx_->increase_stat("Returning cached table metadata");
          LOG_TRACE("already have" << V_(schema) << V_(table));
          return;
        }
      }
      else
      {
        // fast path: when we already have this table
        if( store->has_table(schema, table) )
        {
          server_ctx_->increase_stat("Returning cached metadata list");
          LOG_TRACE("returning cached wildcard data" << V_(schema) << V_(table));
          return;
        }
      }
      
      client_sptr client_copy;
      {
        // save a copy of the client sptr, so that won't disappear
        // while sending the request i.e. reconnect()
        std::unique_lock<std::mutex> l(mtx_);
        if( !client_sptr_ )
        {
          LOG_ERROR("meta client not yet initialized");
          return;
        }
        client_copy = client_sptr_;
      }
      
      if( !client_copy->wait_valid(util::SHORT_TIMEOUT_MS) )
      {
        server_ctx_->increase_stat("No valid upstream metadata server");

        LOG_ERROR("cannot serve request" <<
                  M_(req) <<
                  "because meta client connection to" <<
                  V_(client_copy->server()) <<
                  " timed out in" <<
                  V_(util::SHORT_TIMEOUT_MS));
        return;
      }
      else
      {
        LOG_TRACE("have valid metadata client" <<
                  V_(server_.meta_data_server::rep_base_type::name()) <<
                  V_(server_.meta_data_server::rep_base_type::service_name()) <<
                  V_(schema) <<
                  V_(table));
      }
      
      size_t timeout_ms = 60000;
      server_ctx_->increase_stat("Forwarding metadata request");
      bool send_res = client_copy->send_request(req,
                                                [&](const interface::pb::MetaData & rep)
      {
        for( auto const tm : rep.tables() )
        {
          std::shared_ptr<interface::pb::TableMeta>
            tab_sptr{new interface::pb::TableMeta{tm}};
          
          store->add_table(tab_sptr);
          
          if( tab_sptr->fields_size() == 0 )
          {
            std::string schema_tmp{tm.schema()};
            std::string table_tmp{tm.name()};
            
            // check in 1 sec that we have the full meta_data
            // remove it from the cache if not
            
            timer_service_.schedule(60000,[this,schema_tmp,table_tmp,store]() {
              if( !store->has_fields(schema_tmp, table_tmp) )
              {
                // LOG_TRACE("removing empty data" << V_(schema_tmp) << V_(table_tmp));
                store->remove_table(schema_tmp, table_tmp);
              }
              return false;
            });
          }
        }
        return true;
      }, timeout_ms);
      
      if( send_res )
      {
        LOG_TRACE("successfully forwarded metadat request" <<
                  V_(server_.meta_data_server::rep_base_type::name()) <<
                  V_(server_.meta_data_server::rep_base_type::service_name()) <<
                  V_(schema) <<
                  V_(table));
      }
      else
      {
        server_ctx_->increase_stat("Failed to forward metadata request");
        LOG_ERROR("failed to forward meta-data request to" <<
                  V_(client_copy->server()) <<
                  "within" <<
                  V_(timeout_ms) <<
                  "resetting client connection");
        reset_client();
      }
    });
  }
  
  meta_proxy::~meta_proxy()
  {
    server_.remove_watch();
  }
  
}}

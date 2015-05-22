#include "cfgsvc_mock.hh"
#include <connector/config_server.hh>
#include <connector/endpoint_server.hh>
#include <connector/cert_store_server.hh>
#include <connector/srcsys_credential_server.hh>
#include <connector/cert_store_client.hh>
#include <connector/user_manager_client.hh>
#include <connector/srcsys_credential_client.hh>
#include <util/timer_service.hh>
#include <logger.hh>
#include <future>

using namespace virtdb::test;

struct CfgSvcMock::impl
{
  connector::server_context::sptr            cfg_srv_contex_;
  connector::server_context::sptr            sec_srv_contex_;
  connector::client_context::sptr            clnt_contex_;
  logger::log_sink::socket_sptr              dummy_socket_;
  logger::log_sink::sptr                     sink_stderr_;
  connector::endpoint_server::sptr           ep_srv_;
  connector::endpoint_client::sptr           ep_clnt_;
  connector::config_client::sptr             cfg_clnt_;
  connector::config_server::sptr             cfg_srv_;
  connector::cert_store_server::sptr         certsrv_sptr_;
  connector::srcsys_credential_server::sptr  sscsrv_sptr_;
  util::timer_service                        timer_;
  
  impl(const std::string & ep)
  : cfg_srv_contex_{new connector::server_context},
    sec_srv_contex_{new connector::server_context},
    clnt_contex_{new connector::client_context},
    sink_stderr_{new logger::log_sink{dummy_socket_}}
  {
    // config service context
    cfg_srv_contex_->service_name("config-service");
    cfg_srv_contex_->endpoint_svc_addr(ep);
    cfg_srv_contex_->ip_discovery_timeout_ms(1);
    
    // security service context
    sec_srv_contex_->service_name("security-service");
    sec_srv_contex_->endpoint_svc_addr(ep);
    sec_srv_contex_->ip_discovery_timeout_ms(1);
    
    ep_srv_.reset(new connector::endpoint_server{cfg_srv_contex_});
    ep_clnt_.reset(new connector::endpoint_client{clnt_contex_, ep_srv_->local_ep(), ep_srv_->name()});
    
    cfg_clnt_.reset(new connector::config_client{clnt_contex_, *ep_clnt_, "config-service"});
    
    for( auto const & ep : ep_srv_->endpoint_hosts() )
    {
      cfg_srv_contex_->bind_also_to(ep);
      sec_srv_contex_->bind_also_to(ep);
    }
    
    cfg_srv_contex_->ip_discovery_timeout_ms(10);
    
    cfg_srv_.reset(new connector::config_server{cfg_srv_contex_, *cfg_clnt_});
    
    ep_clnt_->wait_valid_req();
    ep_clnt_->wait_valid_sub();
    cfg_clnt_->wait_valid_req();
    cfg_clnt_->wait_valid_sub();
    
    // initialize security services
    certsrv_sptr_.reset(new connector::cert_store_server{sec_srv_contex_, *cfg_clnt_});
    sscsrv_sptr_.reset(new connector::srcsys_credential_server{sec_srv_contex_, *cfg_clnt_});
    
    {
      // test client to make sure security services are alive
      connector::client_context::sptr cert_ctx{new connector::client_context};
      connector::cert_store_client cert_cli(cert_ctx, *ep_clnt_, "security-service");
      cert_cli.wait_valid();
      
      connector::srcsys_credential_client ssc_cli{cert_ctx, *ep_clnt_, "security-service"};
      ssc_cli.wait_valid();        
    }
    
    timer_.schedule(util::DEFAULT_ENDPOINT_EXPIRY_MS/3, [this]() {
      sec_srv_contex_->keep_alive(*ep_clnt_);
      cfg_srv_contex_->keep_alive(*ep_clnt_);
      return true;
    });
  }
};

CfgSvcMock::CfgSvcMock(const std::string & ep) : impl_(new impl(ep)) {}
CfgSvcMock::~CfgSvcMock() {}
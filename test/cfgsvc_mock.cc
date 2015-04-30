#include "cfgsvc_mock.hh"
#include <connector/config_server.hh>
#include <connector/endpoint_server.hh>
#include <connector/cert_store_server.hh>
#include <connector/srcsys_credential_server.hh>
#include <connector/cert_store_client.hh>
#include <connector/user_manager_client.hh>
#include <connector/srcsys_credential_client.hh>
#include <logger.hh>
#include <future>

using namespace virtdb::test;

struct CfgSvcMock::impl
{
  connector::server_context::sptr      srv_contex_;
  connector::client_context::sptr      clnt_contex_;
  logger::log_sink::socket_sptr        dummy_socket_;
  logger::log_sink::sptr               sink_stderr_;
  connector::endpoint_server::sptr     ep_srv_;
  connector::endpoint_client::sptr     ep_clnt_;
  connector::config_client::sptr       cfg_clnt_;
  connector::config_server::sptr       cfg_srv_;
  
  impl(const std::string & ep)
  : srv_contex_{new connector::server_context},
  clnt_contex_{new connector::client_context},
  sink_stderr_{new logger::log_sink{dummy_socket_}}
  {
    srv_contex_->service_name("config-service");
    srv_contex_->endpoint_svc_addr(ep);
    srv_contex_->ip_discovery_timeout_ms(1);
    
    ep_srv_.reset(new connector::endpoint_server{srv_contex_});
    ep_clnt_.reset(new connector::endpoint_client{clnt_contex_, ep_srv_->local_ep(), ep_srv_->name()});
    
    cfg_clnt_.reset(new connector::config_client{clnt_contex_, *ep_clnt_, "config-service"});
    
    for( auto const & ep : ep_srv_->endpoint_hosts() )
    {
      srv_contex_->bind_also_to(ep);
    }
    
    srv_contex_->ip_discovery_timeout_ms(1);
    
    cfg_srv_.reset(new connector::config_server{srv_contex_, *cfg_clnt_});
    
    ep_clnt_->wait_valid_req();
    ep_clnt_->wait_valid_sub();
    cfg_clnt_->wait_valid_req();
    cfg_clnt_->wait_valid_sub();
  }
};

CfgSvcMock::CfgSvcMock(const std::string & ep) : impl_(new impl(ep)) {}
CfgSvcMock::~CfgSvcMock() {}
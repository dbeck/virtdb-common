#include "datasrc_test.hh"
#include "value_type_test.hh"
#include "logger_test.hh"
#include "util_test.hh"
#include "connector_test.hh"
#include "engine_test.hh"
#include "fault_test.hh"
#include "cfgsvc_mock.hh"
#include <connector/server_context.hh>
#include <connector/client_context.hh>
#include <connector/endpoint_client.hh>
#include <connector/config_client.hh>
#include <fault/injector.hh>

#ifdef CFG_SVC_MOCK_PORT
unsigned short default_cfgsvc_mock_port = CFG_SVC_MOCK_PORT;
#else
unsigned short default_cfgsvc_mock_port = 55441;
#endif

std::string global_mock_ep{"tcp://127.0.0.1:"};

int main(int argc, char **argv)
{
  using namespace virtdb;
  using namespace virtdb::test;
  
  // mock config service EP
  std::string & mock_ep = global_mock_ep;
  mock_ep += std::to_string(default_cfgsvc_mock_port);
  
  std::shared_ptr<connector::client_context>   cli_ctx_;
  std::shared_ptr<CfgSvcMock>                  local_conn_;
  cli_ctx_.reset(new connector::client_context);
  
  local_conn_.reset(new CfgSvcMock(mock_ep));
  
  {
    std::shared_ptr<connector::endpoint_client>  ep_cli_;
    std::shared_ptr<connector::config_client>    cfg_cli_;
    
    ep_cli_.reset(new connector::endpoint_client{
      cli_ctx_,
      mock_ep,
      "config-service"
    });
    cfg_cli_.reset(new connector::config_client{
      cli_ctx_,
      *ep_cli_,
      "config-service"
    });
    ep_cli_->wait_valid_req();
    ep_cli_->wait_valid_sub();
    cfg_cli_->wait_valid_req();
    cfg_cli_->wait_valid_sub();
  }
  
  {
    // initialize fault injector
    auto & global_injector = virtdb::fault::injector::instance();
    global_injector.set_rule(std::string(),
                             0,
                             0,
                             0.0);
  
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();    
    return ret;
  }
}


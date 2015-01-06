#include "datasrc_test.hh"
#include "value_type_test.hh"
#include "logger_test.hh"
#include "util_test.hh"
#include "connector_test.hh"
#include "engine_test.hh"
#include "fault_test.hh"
#include <connector/endpoint_client.hh>
#include <fault/injector.hh>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>

#ifdef CFG_SVC_MOCK_PORT
unsigned short default_cfgsvc_mock_port = CFG_SVC_MOCK_PORT;
#else
unsigned short default_cfgsvc_mock_port = 55441;
#endif

std::string global_mock_ep{"tcp://127.0.0.1:"};

void print_error(const char * str)
{
  std::cerr << str;
  exit(123);
}

int main(int argc, char **argv)
{
  // child process
  const char * progname = "./cfgsvc_mock";
  // mock config service EP
  std::string & mock_ep = global_mock_ep;
  mock_ep += std::to_string(default_cfgsvc_mock_port);

  pid_t pid = fork();
  
  if( pid < 0 )
  {
    // parent
    print_error("fork failed");
  }
  else if( pid == 0 )
  {
    // execute program
    execl(progname, progname, mock_ep.c_str(), 0);
    print_error("execle error");
  }
  else
  {
    // initialize fault injector
    auto & global_injector = virtdb::fault::injector::instance();
    global_injector.set_rule(std::string(),
                             0,
                             0,
                             0.0);
  
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    
    // stop mock service
    {
      using namespace virtdb;
      using namespace virtdb::interface;
      connector::endpoint_client ep_clnt(mock_ep,  "STOP");
      
      pb::EndpointData ep_data;
      {
        ep_data.set_name("STOP");
        ep_data.set_svctype(pb::ServiceType::OTHER);
        
        auto conn = ep_data.add_connections();
        conn->add_address("invalid address");
        conn->set_type(pb::ConnectionType::REQ_REP);
        
        ep_clnt.register_endpoint(ep_data);
      }
    }
        
    int stat = 0;
    waitpid(pid, &stat, 0);
    return ret;
  }
}


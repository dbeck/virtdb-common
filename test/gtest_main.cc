#include "datasrc_test.hh"
#include "value_type_test.hh"
#include "logger_test.hh"
#include "util_test.hh"
#include "connector_test.hh"
#include "engine_test.hh"
#include "fault_test.hh"
#include "cachedb_test.hh"
#include <fault/injector.hh>

#ifdef CFG_SVC_MOCK_PORT
unsigned short default_cfgsvc_mock_port = CFG_SVC_MOCK_PORT;
#else
unsigned short default_cfgsvc_mock_port = 55441;
#endif

std::string global_mock_ep{"tcp://127.0.0.1:"};

int main(int argc, char **argv)
{
  // child process
  const char * progname = "./cfgsvc_mock";
  // mock config service EP
  std::string & mock_ep = global_mock_ep;
  mock_ep += std::to_string(default_cfgsvc_mock_port);

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


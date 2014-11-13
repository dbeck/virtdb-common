#include "datasrc_test.hh"
#include "value_type_test.hh"
#include "logger_test.hh"
#include "util_test.hh"
#include "connector_test.hh"
#include "engine_test.hh"
#include "fault_test.hh"
#include <fault/injector.hh>

int main(int argc, char **argv)
{
  auto & global_injector = virtdb::fault::injector::instance();
  global_injector.set_rule(std::string(),
                           0,
                           0,
                           0.0);
  
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


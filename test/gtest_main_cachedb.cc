#include "cachedb_test.hh"
#include <fault/injector.hh>

int main(int argc, char **argv)
{
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


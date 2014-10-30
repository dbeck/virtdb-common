#include "datasrc_test.hh"
#include "value_type_test.hh"
#include "logger_test.hh"
#include "util_test.hh"
#include "connector_test.hh"

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


#include "fault_test.hh"
#include <fault/injector.hh>
#include <util/exception.hh>
#include <iostream>

using namespace virtdb::util;
using namespace virtdb::test;
using namespace virtdb::fault;

TEST_F(InjectorTest, NeverFaultyRule)
{
  injector inj;
  inj.set_rule("never_faulty_test", 0, 0, 0.0);
  for( int i=0; i<1000000; ++i )
  {
    EXPECT_FALSE(inj.inject("never_faulty_test", "test"));
  }
}

TEST_F(InjectorTest, TenPercentFault)
{
  size_t n_faults = 0;
  injector inj;
  inj.set_rule("ten_percent_faults", 0, 0, 0.1);
  for( int i=0; i<1000000; ++i )
  {
    if(inj.inject("ten_percent_faults", "test"))
      ++n_faults;
  }
  EXPECT_GT(n_faults, 0);
  EXPECT_LT(n_faults, 110000);
  std::cout << "n_faults=" << n_faults << "\n";
}

TEST_F(InjectorTest, AlwaysFaultyRule)
{
  size_t n_faults = 0;
  injector inj;
  inj.set_rule("always_faulty_test", 0, 0, 1.0);
  for( int i=0; i<1000000; ++i )
  {
    if(inj.inject("always_faulty_test", "test"))
      ++n_faults;
  }
  EXPECT_EQ(n_faults, 1000000);
}

TEST_F(InjectorTest, LongLeadTime)
{
  size_t n_faults = 0;
  injector inj;
  inj.set_rule("long_lead_time", 100000000, 0, 1.0);
  for( int i=0; i<1000000; ++i )
  {
    if(inj.inject("long_lead_time", "test"))
      ++n_faults;
  }
  EXPECT_EQ(n_faults, 0);
}

TEST_F(InjectorTest, LongPeriodicity)
{
  size_t n_faults = 0;
  injector inj;
  inj.set_rule("long_periodicity", 0, 100000000, 1.0);
  for( int i=0; i<1000000; ++i )
  {
    if(inj.inject("long_periodicity", "test"))
      ++n_faults;
  }
  EXPECT_EQ(n_faults, 1);
}

#include "value_type_test.hh"
#include <list>
#include <set>
#include <deque>
#include <vector>

using namespace virtdb::test;

TEST_F(ValueTypeTest, TestString)
{
  std::vector<std::string> values{"apple","peach","lemon"};
  // TODO TestString: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestI32)
{
  std::set<int32_t> values{1,3,-5,6};
  // TODO TestI32: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestI64)
{
  std::list<int64_t> values{-6,9,-123};
  // TODO TestI64: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestU32)
{
  std::deque<uint32_t> values{98,32,11};
  // TODO TestU32: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestU64)
{
  std::vector<uint64_t> values{2,66,89,123};
  // TODO TestU64: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestDouble)
{
  std::set<double> values{1.34,999.876,123.765};
  // TODO TestDouble: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestFloat)
{
  std::list<float> values{13.4,111.23,998.23};
  // TODO TestFloat: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestBool)
{
  std::deque<bool> values{true, false, false, true};
  // TODO TestBool: get, set, size, iterate
}

TEST_F(ValueTypeTest, TestNulls)
{
  std::deque<bool> values{true, false, false, true};
  // TODO TestNulls
}



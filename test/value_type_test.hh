#pragma once

#include <gtest/gtest.h>
#include <value_type.hh>
#include <memory>

namespace virtdb {
namespace test {

  class ValueTypeTest : public ::testing::Test
  {
  protected:
    typedef std::shared_ptr<virtdb::interface::pb::ValueType> value_type_sptr;
    value_type_sptr value_type_sptr_;
    
    ValueTypeTest() : value_type_sptr_(new virtdb::interface::pb::ValueType) {}

    virtual void SetUp() { }
    virtual void TearDown() {}

    std::string toString()
    {
      return value_type_sptr_->DebugString();
    }
  };

} } // virtdb::test


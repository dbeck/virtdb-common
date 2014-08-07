#pragma once

#include <gtest/gtest.h>
#include <memory>

namespace virtdb { namespace test {
  
  class UtilTest : public ::testing::Test
  {
  protected:
    virtual void SetUp() { }
    virtual void TearDown() {}
  };
  
}}

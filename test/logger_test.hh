#pragma once

#include <gtest/gtest.h>
#include "../logger.hh"
#include <memory>

namespace virtdb { namespace test {

  class LoggerTest : public ::testing::Test
  {
  protected:
    virtual void SetUp() { }
    virtual void TearDown() {}
  };

}}
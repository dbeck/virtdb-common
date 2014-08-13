#pragma once

#include <gtest/gtest.h>
#include <memory>

namespace virtdb { namespace test {
  
  class ActiveQueueTest : public ::testing::Test { };
  class BarrierTest : public ::testing::Test { };
  class RelativeTimeTest : public ::testing::Test { };
  class ExceptionTest : public ::testing::Test { };
  class NetTest : public ::testing::Test { };
  
}}


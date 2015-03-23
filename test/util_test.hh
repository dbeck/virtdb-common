#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <util/active_queue.hh>
#include <util/barrier.hh>
#include <atomic>

namespace virtdb { namespace test {
  
  class UtilActiveQueueTest : public ::testing::Test
  {
  protected:
    UtilActiveQueueTest();
    
    std::atomic<int> value_;
    util::active_queue<int,100> queue_;
  };
  
  class UtilBarrierTest : public ::testing::Test
  {
  protected:
    UtilBarrierTest();
    
    util::barrier barrier_;
  };
  
  class UtilNetTest : public ::testing::Test { };
  class UtilFlexAllocTest : public ::testing::Test { };
  class UtilAsyncWorkerTest : public ::testing::Test { };
  class UtilCompareMessagesTest : public ::testing::Test { };
  class UtilZmqTest : public ::testing::Test { };
  class UtilTableCollectorTest : public ::testing::Test { };
  class UtilUtf8Test : public ::testing::Test { };
  class UtilHashFileTest : public ::testing::Test { };
}}


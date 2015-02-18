#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <util/active_queue.hh>
#include <util/barrier.hh>
#include <atomic>

namespace virtdb { namespace test {
  
  class ActiveQueueTest : public ::testing::Test
  {
  protected:
    ActiveQueueTest();
    
    std::atomic<int> value_;
    util::active_queue<int,100> queue_;
  };
  
  class BarrierTest : public ::testing::Test
  {
  protected:
    BarrierTest();
    
    util::barrier barrier_;
  };
  
  class NetTest : public ::testing::Test { };
  class FlexAllocTest : public ::testing::Test { };
  class AsyncWorkerTest : public ::testing::Test { };
  class CompareMessagesTest : public ::testing::Test { };
  class ZmqTest : public ::testing::Test { };
  class TableCollectorTest : public ::testing::Test { };
  class Utf8Test : public ::testing::Test { };
}}


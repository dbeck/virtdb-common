#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include "../logger.hh"

namespace virtdb { namespace test {

  class LoggerTest : public ::testing::Test
  {
  protected:
    virtual void SetUp();
    virtual void TearDown();
    LoggerTest();

    bool init_zmq_ctx();
    bool init_zmq_sink();
    bool init_zmq_receiver();
    
    void shutdown_receiver();
    void receiver_entry();
    
    static const char * inproc_endpoint();
    static const char * ipc_endpoint();
    
    std::shared_ptr<zmq::context_t>  sink_ctx_;
    logger::log_sink::log_sink_sptr  sink_sptr_;
    logger::log_sink::socket_sptr    control_sptr_;
    logger::log_sink::socket_sptr    receiver_sptr_;
    std::atomic<uint32_t>            n_received_;
    std::thread                      receiver_thread_;
    
    std::mutex                       expected_lock_;
    std::condition_variable          expected_cv_;
    void signal_new_message();
    bool wait_for_messages(uint32_t expected_messages,
                           uint32_t timeout_ms);
  };
  
  class HeaderStoreTest : public ::testing::Test { };
  class SymbolStoreTest : public ::testing::Test { };
}}

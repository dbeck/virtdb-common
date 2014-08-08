#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <future>
#include <atomic>
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
    bool send_empty_record();
    
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
    std::future<bool>                received_message_;
    std::promise<bool>               message_promise_;
  };
}}

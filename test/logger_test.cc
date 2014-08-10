#include "logger_test.hh"
#include <iostream>

using namespace virtdb::test;
using namespace virtdb::logger;
using namespace virtdb::interface;

const char *
LoggerTest::inproc_endpoint()
{
  return "inproc://#1";
}

const char *
LoggerTest::ipc_endpoint()
{
  return "ipc:///tmp/ipc-endpoint-1";
}

void LoggerTest::SetUp()
{
  init_zmq_ctx();
}

void LoggerTest::TearDown()
{
  shutdown_receiver();
  if( receiver_thread_.joinable() )
    receiver_thread_.join();
}

LoggerTest::LoggerTest() : n_received_(0)
{
  received_message_ = message_promise_.get_future();
}

bool
LoggerTest::init_zmq_ctx()
{
  sink_ctx_.reset(new zmq::context_t(1));
}

void
LoggerTest::shutdown_receiver()
{
  if( control_sptr_ )
  {
    control_sptr_->send("Q", 1);
  }
}

bool
LoggerTest::init_zmq_sink()
{
  sink_sptr_.reset(new logger::log_sink(control_sptr_));
  return true;
}

bool
LoggerTest::init_zmq_receiver()
{
  using logger::log_sink;
  receiver_sptr_.reset(new zmq::socket_t(*sink_ctx_, ZMQ_PULL));
  receiver_sptr_->bind(inproc_endpoint());
  control_sptr_.reset(new zmq::socket_t(*sink_ctx_, ZMQ_PUSH));
  control_sptr_->connect(inproc_endpoint());
  receiver_thread_ = std::move(std::thread(std::bind(&LoggerTest::receiver_entry,this)));
  return true;
}

bool
LoggerTest::send_empty_record()
{
  using logger::log_sink;
  log_sink::pb_logrec_sptr rec(new pb::LogRecord);
  auto process = rec->mutable_process();
  process->set_startdate(1);
  process->set_starttime(2);
  process->set_pid(3);
  process->set_random(4);
  auto sink = log_sink::get_sptr();
  if( sink )
    return sink->send_record(rec);
  return true;
}

void
LoggerTest::receiver_entry()
{
  pb::LogRecord rec;
  try
  {
    while( true )
    {
      zmq::message_t message;
      if( !receiver_sptr_->recv(&message) )
        break;
    
      rec.Clear();
      if( message.size() == 1 &&
          static_cast<const char *>(message.data())[0] == 'Q' )
      {
        break;
      }
      rec.ParseFromArray(message.data(), message.size());
      ++n_received_;
      std::cout << "Received #" << n_received_ <<'[' << message.size() << ']'
                << ": \n" << rec.DebugString() << "\n";
      if( n_received_ == 1 )
        message_promise_.set_value(true);
    }
  }
  catch(const std::exception & e)
  {
    std::cout << "!! Caught exception: " << e.what() << "\n";
  }
}

// TODO ---------------------

TEST_F(LoggerTest, LogInfo)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  std::string username("me");
  for(int i=0;i<2;++i)
  {
    LOG_INFO("test" << V_(username) << "loggged in, foobar=" << 123);
  }
  
  symbol_store::for_each( [](const std::string & symbol_str,
                             uint32_t symbol_id) {
    std::cout << "Symbol[" << symbol_id << "]=" << symbol_str << "\n";
    return true;
  });
  
  received_message_.wait();
}

TEST_F(LoggerTest, LogError)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  LOG_ERROR("error message");
  
  received_message_.wait();
}

TEST_F(LoggerTest, LogTrace)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  LOG_TRACE("trace message");

  received_message_.wait();
}

TEST_F(LoggerTest, LogScoped)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  LOG_SCOPED("scoped message");
  
  received_message_.wait();
}

// OK ----------------------

TEST_F(LoggerTest, SendEmptyRecordWithoutInit)
{
  EXPECT_TRUE(this->send_empty_record());
}

TEST_F(LoggerTest, InitZmqReceiver)
{
  EXPECT_TRUE(this->init_zmq_receiver());
}

TEST_F(LoggerTest, SendEmptyRecordWithZmq)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  EXPECT_TRUE(this->send_empty_record());
  // need to synchronize, otherwise we shutdown the active queue
  // and our thread before it has a chance to handle our dummy message
  received_message_.wait();
}


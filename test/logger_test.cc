#include "logger_test.hh"
#include "util/relative_time.hh"
#include <iostream>
#include <chrono>
#include <set>
#include <map>

using namespace virtdb::test;
using namespace virtdb::logger;
using namespace virtdb::interface;

// #define PRINT_LOG_MESSAGES

void
LoggerTest::signal_new_message()
{
  std::unique_lock<std::mutex> lock(expected_lock_);
  expected_cv_.notify_all();
}

bool
LoggerTest::wait_for_messages(uint32_t expected_messages,
                              uint32_t timeout_ms)
{
  uint64_t start      = util::relative_time().instance().get_msec();
  uint64_t act_time   = start;
  
  while( n_received_ < expected_messages )
  {
    std::unique_lock<std::mutex> lock(expected_lock_);
    expected_cv_.wait_for(lock,
                          std::chrono::milliseconds(100));
    act_time = util::relative_time().instance().get_msec();
    if( act_time  > (start+timeout_ms) )
    {
      return false;
    }
  }
  return true;
}

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
}

bool
LoggerTest::init_zmq_ctx()
{
  sink_ctx_.reset(new zmq::context_t(1));
  return true;
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
      size_t data_len = 0;
      for( int i=0; i<rec.data_size(); ++i )
      {
        data_len += rec.data(i).ByteSize();
      }
#ifdef PRINT_LOG_MESSAGES
      std::cout << "Received #" << n_received_ <<'[' << message.size() << ']'
                << " ndata=" << rec.data_size() << " len=" << data_len
                << ": \n" << rec.DebugString() << "\n";
#endif
      ++n_received_;
      signal_new_message();
    }
  }
  catch(const std::exception & e)
  {
    std::cout << "!! Caught exception: " << e.what() << "\n";
  }
}

TEST_F(LoggerTest, LogInfo)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  std::string username("me");
  for(int i=0;i<2;++i)
  {
    LOG_INFO("test" << V_(username) << "loggged in, foobar=" << 123);
  }
  
  EXPECT_TRUE( this->wait_for_messages(2,3000) );
}

TEST_F(LoggerTest, LogError)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  LOG_ERROR("error message");
  
  EXPECT_TRUE( this->wait_for_messages(1,3000) );
}

TEST_F(LoggerTest, LogTrace)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  LOG_TRACE("trace message");

  EXPECT_TRUE( this->wait_for_messages(1,3000) );
}

TEST_F(LoggerTest, LogScoped)
{
  EXPECT_TRUE(this->init_zmq_receiver());
  EXPECT_TRUE(this->init_zmq_sink());
  
  for(int i=0;i<2;++i)
  {
    // scoped message has to be inside a block
    // in this special test case, because the destructor
    // of the test may destroy the log receiver before
    // the message reaches it, because of the logger's
    // asynch nature
    LOG_SCOPED("scoped message");
  }
  
  EXPECT_TRUE( this->wait_for_messages(4,3000) );
}

TEST_F(LoggerTest, InitZmqReceiver)
{
  EXPECT_TRUE(this->init_zmq_receiver());
}

TEST_F(HeaderStoreTest, NewIdUnique)
{
  log_record * records[] = {
    reinterpret_cast<log_record *>(1),
    reinterpret_cast<log_record *>(2),
    reinterpret_cast<log_record *>(3)
  };
  
  uint32_t record_ids[] = {
    header_store::get_new_id(records[0]),
    header_store::get_new_id(records[1]),
    header_store::get_new_id(records[2])
  };
  
  EXPECT_TRUE(header_store::has_header(records[0]));
  EXPECT_TRUE(header_store::has_header(records[1]));
  EXPECT_TRUE(header_store::has_header(records[2]));
  
  EXPECT_NE(record_ids[0], record_ids[1]);
  EXPECT_NE(record_ids[0], record_ids[2]);
  EXPECT_NE(record_ids[1], record_ids[2]);
}

TEST_F(HeaderStoreTest, NonExistantRecordAndId)
{
  log_record * record = reinterpret_cast<log_record *>(0xDeadBeaf);
  
  EXPECT_FALSE(header_store::has_header(record));
  EXPECT_FALSE(header_store::header_sent(0xDeadBeaf));
  
  uint32_t dead_beaf_id = header_store::get_new_id(record);
  
  EXPECT_TRUE(header_store::has_header(record));
  EXPECT_FALSE(header_store::header_sent(dead_beaf_id));
  
  header_store::header_sent(dead_beaf_id,true);
  EXPECT_TRUE(header_store::header_sent(dead_beaf_id));
  
  header_store::header_sent(0xDeadBeaf,true);
  EXPECT_TRUE(header_store::header_sent(0xDeadBeaf));
}

TEST_F(SymbolStoreTest, SymbolMapping)
{
  uint32_t id0 = symbol_store::get_symbol_id("MapMe");
  uint32_t id1 = symbol_store::get_symbol_id("MapMeToo");
  uint32_t min_id = std::min(id0, id1);
  
  // basic id checks
  EXPECT_NE(id0, 0);
  EXPECT_NE(id1, 0);
  EXPECT_NE(id0, id1);
  
  // check has_more function
  EXPECT_TRUE(symbol_store::has_more(0));
  EXPECT_TRUE(symbol_store::has_more(1));
  
  uint32_t max_id = symbol_store::max_id_sent();
  symbol_store::max_id_sent(min_id);
  
  // make sure we have what we have set
  EXPECT_EQ(symbol_store::max_id_sent(), min_id);
  
  {
    std::map<std::string, uint32_t> symbol_map;
    
    // iterate all over symbols and store the data into symbol_map
    symbol_store::for_each([&symbol_map](const std::string & symbol_string, uint32_t symbol_id) {
      symbol_map[symbol_string] = symbol_id;
      return true;
    }, 0);
    
    // check we received our symbols with the right id
    EXPECT_EQ( symbol_map["MapMe"], id0 );
    EXPECT_EQ( symbol_map["MapMeToo"], id1 );
  }

  {
    std::map<std::string, uint32_t> symbol_map;
    
    // iterate all over the symbols > min_id-1 and store the data into symbol_map
    symbol_store::for_each([&symbol_map](const std::string & symbol_string, uint32_t symbol_id) {
      symbol_map[symbol_string] = symbol_id;
      return true;
    }, min_id-1);
    
    // check we received our symbols with the right id
    EXPECT_EQ( symbol_map["MapMe"], id0 );
    EXPECT_EQ( symbol_map["MapMeToo"], id1 );
  }

  // restore max id
  symbol_store::max_id_sent(max_id);
}


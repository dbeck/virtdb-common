#include "util_test.hh"
#include "util.hh"
#include <future>
#include <thread>

using namespace virtdb::test;
using namespace virtdb::util;

ActiveQueueTest::ActiveQueueTest()
: value_(0),
  queue_(10,[this](int v){ value_ += v; })
{
}

TEST_F(ActiveQueueTest, AddNumbers)
{
  for( int i=1; i<=100; ++i )
    this->queue_.push(i);

  EXPECT_TRUE( this->queue_.wait_empty(std::chrono::milliseconds(100)) );
  this->queue_.stop();
  EXPECT_TRUE( this->queue_.stopped() );
  EXPECT_EQ( this->value_, 5050 );
}

TEST_F(ActiveQueueTest, Stop3Times)
{
  this->queue_.stop();
  EXPECT_TRUE( this->queue_.stopped() );
  this->queue_.stop();
  EXPECT_TRUE( this->queue_.stopped() );
  this->queue_.stop();
  EXPECT_TRUE( this->queue_.stopped() );
}

BarrierTest::BarrierTest() : barrier_(10) {}

TEST_F(BarrierTest, BarrierReady)
{
  std::atomic<int> flag{0};
  std::vector<std::thread> threads;
  
  // start 9 threads an make sure they are all waiting
  for( int i=0; i<9; ++i )
  {
    EXPECT_FALSE( this->barrier_.ready() );
    auto & b = this->barrier_;
    threads.push_back(std::thread{[&b,&flag](){
      b.wait();
      ++flag;
    }});
    // all threads are waiting
    EXPECT_EQ(flag, 0);
    EXPECT_FALSE(b.ready());
  }
  
  // let the detached threads go by doing a wait on the barrier
  this->barrier_.wait();
  EXPECT_TRUE( this->barrier_.ready() );

  // cleanup threads
  for( auto & t : threads )
  {
    if( t.joinable() )
      t.join();
  }
  
  EXPECT_EQ(flag, 9);
}

TEST_F(NetTest, DummyTest)
{
  // TODO : NetTest
}

TEST_F(FlexAllocTest, DummyTest)
{
  // TODO : FlexAllocTest
}

TEST_F(AsyncWorkerTest, DummyTest)
{
  // TODO : AsyncWorkerTest
}

TEST_F(CompareMessagesTest, DummyTest)
{
  // TODO : CompareMessagesTest
}

TEST_F(ZmqTest, TestValid)
{
  
  zmq::context_t rep_ctx(1);
  zmq_socket_wrapper srv(rep_ctx,ZMQ_REP);
  EXPECT_FALSE(srv.valid());

  {
    std::promise<bool> prom;
    std::future<bool> fut(prom.get_future());

    std::thread valid_thread([&](){
      prom.set_value(srv.wait_valid(100));
    });
    
    srv.bind("tcp://127.0.0.1:*");
    EXPECT_TRUE(srv.valid());
    fut.wait();
    bool is_valid = fut.get();
    EXPECT_TRUE(is_valid);
    EXPECT_TRUE(valid_thread.joinable());
    valid_thread.join();
  }
  
  { // test-connect
    zmq::context_t req_ctx(1);
    zmq_socket_wrapper clnt(req_ctx, ZMQ_REQ);
    EXPECT_FALSE(clnt.valid());
    
    {
      std::promise<bool> prom;
      std::future<bool> fut(prom.get_future());
      
      std::thread valid_thread([&](){
        prom.set_value(clnt.wait_valid(100));
      });
      
      for( auto const & ep : srv.endpoints() )
      {
        try
        {
          clnt.connect(ep.c_str());
          std::cerr << "connected to: " << ep << "\n";
        } catch( ... ) { }
      }
      
      EXPECT_TRUE(clnt.valid());
      fut.wait();
      bool is_valid = fut.get();
      EXPECT_TRUE(is_valid);
      EXPECT_TRUE(valid_thread.joinable());
      valid_thread.join();
    }
  }

  { // test-reconnect
    zmq::context_t req_ctx(1);
    zmq_socket_wrapper clnt(req_ctx, ZMQ_REQ);
    EXPECT_FALSE(clnt.valid());
    
    {
      std::promise<bool> prom;
      std::future<bool> fut(prom.get_future());
      
      std::thread valid_thread([&](){
        prom.set_value(clnt.wait_valid(100));
      });
      
      for( auto const & ep : srv.endpoints() )
      {
        try
        {
          clnt.reconnect(ep.c_str());
          std::cerr << "connected to: " << ep << "\n";
        } catch( ... ) { }
      }
      
      EXPECT_TRUE(clnt.valid());
      fut.wait();
      bool is_valid = fut.get();
      EXPECT_TRUE(is_valid);
      EXPECT_TRUE(valid_thread.joinable());
      valid_thread.join();
    }
  }
  
  /*
  auto parsed = parse_zmq_tcp_endpoint("tcp://hello-world:1234");
   */
}


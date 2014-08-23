#include "util_test.hh"
#include "util.hh"

using namespace virtdb::test;
using namespace virtdb::util;

TEST_F(BarrierTest, DummyTest)
{
  // TODO : BarrierTest
}

TEST_F(ActiveQueueTest, DummyTest)
{
  // TODO : ActiveQueueTest
}

TEST_F(RelativeTimeTest, DummyTest)
{
  // TODO : RelativeTimeTest
}

TEST_F(ExceptionTest, DummyTest)
{
  // TODO : ExceptionTest
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

TEST_F(ZmqTest, DummyTest)
{
  // TODO : ZmqTest
  zmq::context_t ctx(1);
  zmq_socket_wrapper sock(ctx,ZMQ_REP);
  sock.bind("tcp://127.0.0.1:*");
  
  auto parsed = parse_zmq_tcp_endpoint("tcp://hello-world:1234");
}


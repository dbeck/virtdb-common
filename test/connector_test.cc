#include "connector_test.hh"
#include <logger.hh>
#include <connector/endpoint_client.hh>
#include <atomic>
#include <thread>
#include <util/barrier.hh>
#include <util/exception.hh>

using namespace virtdb::connector;
using namespace virtdb::test;
using namespace virtdb::interface;
using namespace virtdb::util;

extern std::string global_mock_ep;

#if 0
// req_client base has:
// --------------------
// bool send_request(const req_item & req,
//                  std::function<bool(const rep_item & rep)> cb,
//                  unsigned long timeout_ms,
//                  std::function<void(void)> on_timeout=[]{})

// sub_client base has:
// --------------------
// void watch(const std::string & subscription,
//            sub_monitor m);
// void remove_watches();
// void remove_watch(const std::string & subscription);
#endif


TEST_F(ConfigClientTest, RemoveNonexistentWatches)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-RemoveNonexistentWatches";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");
  
  // this is OK:
  cfg_clnt.remove_watches();
}

TEST_F(ConfigClientTest, DoubleCleanupRethrow)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-DoubleCleanupRethrow";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");
  
  // this sequence is valid, rethrow should throw excpetions here
  cfg_clnt.cleanup();
  cfg_clnt.rethrow_error();
  cfg_clnt.cleanup();
  cfg_clnt.rethrow_error();
}

TEST_F(ConfigClientTest, TripleRethrow)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-TripleRethrow";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");
  
  // rethrow should survive this without exception or crash
  cfg_clnt.rethrow_error();
  cfg_clnt.rethrow_error();
  cfg_clnt.rethrow_error();
}

TEST_F(ConfigClientTest, TripleCleanup)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-TripleCleanup";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");

  // cleanup should survive this without exception or crash
  cfg_clnt.cleanup();
  cfg_clnt.cleanup();
  cfg_clnt.cleanup();
}

TEST_F(ConfigClientTest, BadServiceNameToConnect)
{
  // this should succeed without exception and error
  const char * name = "ConfigClientTest-BadServiceNameToConnect";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "nope-config-service");
  
  // this should fail because no such service exists
  EXPECT_FALSE(cfg_clnt.wait_valid_sub(200));
  EXPECT_FALSE(cfg_clnt.wait_valid_req(200));
}

TEST_F(ConfigClientTest, SimpleConnect)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-ConnectOnly";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");
  
  // this should succeed assuming the mock service is running
  EXPECT_TRUE(cfg_clnt.wait_valid_sub(200));
  EXPECT_TRUE(cfg_clnt.wait_valid_req(200));
}

TEST_F(ConfigClientTest, CheckReqChannel) { EXPECT_TRUE(false); }
TEST_F(ConfigClientTest, CheckSubChannel) { EXPECT_TRUE(false); }



TEST_F(ColumnClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ColumnServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(ConfigServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(DbConfigClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(DbConfigServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(EndpointServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(IpDiscoveryClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(IpDiscoveryServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(LogRecordClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(LogRecordServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(MetaDataClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(MetaDataServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(SubClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(PubServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(PushClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(PullServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(QueryClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(QueryServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(ReqClientTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(RepServerTest, ImplementMe) { EXPECT_TRUE(false); }

TEST_F(ServerBaseTest, ConstuctHostSet)
{
  const char * name = "ServerBaseTest-ConstuctHostSet";
  endpoint_client   ep_clnt(global_mock_ep, name);
  config_client     cfg_clnt(ep_clnt, "config-service");

  server_base bs(cfg_clnt);
  auto const & host_set{bs.hosts()};
}

TEST_F(EndpointClientTest, StressWatch)
{
  const char * name = "EndpointClientTest-StressWatch";
  endpoint_client ep_clnt(global_mock_ep, name);

  auto watch = [](const pb::EndpointData & ep) {};

  for( int i=0; i<300; ++i )
    ep_clnt.watch(pb::ServiceType::OTHER, watch);
  
  for( int i=0; i<200; ++i )
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    // long lifetime for this endpoint
    ep_data.set_validforms(10000);
    ep_clnt.register_endpoint(ep_data);
  }
  
  {
    pb::EndpointData result;
    EXPECT_TRUE(ep_clnt.get(ep_clnt.name(),pb::ServiceType::OTHER,result));
  }
  
  for( int i=0; i<300; ++i )
    ep_clnt.watch(pb::ServiceType::OTHER, watch);
  
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    // very short lifetime for this endpoint to cleanuo
    ep_data.set_validforms(1);
    ep_clnt.register_endpoint(ep_data);
    
  }
}

TEST_F(EndpointClientTest, StressRegister)
{
  const char * name = "EndpointClientTest-StressRegister";
  
  endpoint_client ep_clnt(global_mock_ep, name);
  pb::EndpointData ep_data;
  
  ep_data.set_svctype(pb::ServiceType::OTHER);
  auto conn = ep_data.add_connections();
  conn->add_address("test-address");
  conn->set_type(pb::ConnectionType::REQ_REP);
 
  for( int ms=1; ms<1025; ms = ms*2 )
  {
    ep_data.set_validforms(ms);
    
    for( int i=0; i<20; ++i )
    {
      std::string name_tmp{name};
      name_tmp += std::to_string(ms) + "-" + std::to_string(i);
      
      ep_data.set_name(name_tmp);
      
      ep_clnt.register_endpoint(ep_data);
    }
  }
}

TEST_F(EndpointClientTest, InvalidRegister)
{
  const char * name = "EndpointClientTest-InvalidRegister";
  
  endpoint_client ep_clnt(global_mock_ep, name);

  pb::EndpointData ep_data;
  // empty EndpointData
  EXPECT_THROW(ep_clnt.register_endpoint(ep_data), virtdb::util::exception);

  // missing name
  ep_data.set_validforms(1);
  EXPECT_THROW(ep_clnt.register_endpoint(ep_data), virtdb::util::exception);
  
  {
    pb::EndpointData result;
    EXPECT_FALSE(ep_clnt.get(ep_data.name(),pb::ServiceType::OTHER,result));
  }
  
  // missing connection type
  ep_data.set_name(ep_clnt.name());
  ep_data.set_svctype(pb::ServiceType::OTHER);
  auto conn = ep_data.add_connections();
  conn->add_address("test-address");
  EXPECT_THROW(ep_clnt.register_endpoint(ep_data), virtdb::util::exception);
  
  {
    pb::EndpointData result;
    EXPECT_FALSE(ep_clnt.get(ep_data.name(),pb::ServiceType::OTHER,result));
  }
}

TEST_F(EndpointClientTest, InvalidWatch)
{
  const char * name = "EndpointClientTest-InvalidWatch";
  
  endpoint_client ep_clnt(global_mock_ep, name);
  std::function<void(const pb::EndpointData &)> empty_watch;
  
  EXPECT_THROW(ep_clnt.watch(pb::ServiceType::OTHER, empty_watch), virtdb::util::exception);
  
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    // very short lifetime for this endpoint
    ep_data.set_validforms(1);
    
    // throw in the synchronous monitor
    EXPECT_THROW(ep_clnt.register_endpoint(ep_data, empty_watch), virtdb::util::exception);
  }
}

TEST_F(EndpointClientTest, InvalidConstr)
{
  auto invalid_svc_config_ep = []() {
    endpoint_client ep_clnt("", "EndpointClientTest-InvalidConstr");
  };
  EXPECT_THROW(invalid_svc_config_ep(), virtdb::util::exception);
  
  auto invalid_service_name = []() {
    endpoint_client ep_clnt("tcp://0.0.0.0:0", "");
  };
  EXPECT_THROW(invalid_service_name(), virtdb::util::exception);
  
  auto malformed_ep = []() {
    endpoint_client ep_clnt("invalid ep", "xxx");
  };
  EXPECT_THROW(malformed_ep(), virtdb::util::exception);
}

TEST_F(EndpointClientTest, Watch)
{
  barrier on_callback{2};
  
  auto ep_callback = [&on_callback](const pb::EndpointData & ep)
  {
    if( ep.name() == "EndpointClientTest-Watch" &&
       ep.svctype() == pb::ServiceType::OTHER )
    {
      for( auto & conn : ep.connections() )
      {
        if( conn.type() == pb::ConnectionType::REQ_REP )
        {
          for( auto & addr : conn.address() )
          {
            if( addr.find("test-address") == 0 )
            {
              on_callback.wait();
            }
          }
        }
      }
    }
  };
  
  endpoint_client ep_clnt(global_mock_ep, "EndpointClientTest-Watch");
  ep_clnt.watch(pb::OTHER, ep_callback);
  
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    ep_clnt.register_endpoint(ep_data);

    // the async callback should fire and release the barrier
    EXPECT_TRUE(on_callback.wait_for(200));

    // check client cache
    {
      pb::EndpointData result;
      EXPECT_TRUE(ep_clnt.get(ep_data.name(),pb::ServiceType::OTHER,result));
    }
    
    // cleanup
    ep_data.set_validforms(1);
    ep_clnt.register_endpoint(ep_data);
    
    // give a bit of time for expiry
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_F(EndpointClientTest, Expiry)
{
  const char * name = "EndpointClientTest-Expiry";
  
  endpoint_client ep_clnt(global_mock_ep, name);
  
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);

    // very short lifetime for this endpoint
    ep_data.set_validforms(1);
    
    ep_clnt.register_endpoint(ep_data);
  }
  
  {
    // give a bit of time to the server for the expiry
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    
    // check endpoints
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::NONE);
    
    std::atomic_int counter{0};
    ep_clnt.register_endpoint(ep_data,[name,&counter](const pb::EndpointData & ep)
                              {
                                // make sure we don't get back the expired EP data
                                EXPECT_NE(ep.name(), name);
                                ++counter;
                              });
    EXPECT_GT(counter, 0);
  }
}

TEST_F(EndpointClientTest, MonitorException)
{
  const char * name = "EndpointClientTest-MonitorException";
  
  endpoint_client ep_clnt(global_mock_ep, name);
  
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    // very short lifetime for this endpoint
    ep_data.set_validforms(1);
    
    // throw in the synchronous monitor
    ep_clnt.register_endpoint(ep_data,[](const pb::EndpointData & ep)
    {
      THROW_("test exception");
    });
  }

  {
    std::atomic_bool throw_exc{false};
    auto fun = [&throw_exc](const pb::EndpointData & ep)
    {
      if( throw_exc )
      {
        THROW_("test exception 2");
      }
    };
    
    ep_clnt.watch(pb::ServiceType::OTHER, fun);
    
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address 2");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    // 1s lifetime for this endpoint
    ep_data.set_validforms(1000);
    
    // throw in the asynchronous monitor
    throw_exc = true;
    ep_clnt.register_endpoint(ep_data);
    
    // very short lifetime for this endpoint
    // for cleanup purposes
    ep_data.set_validforms(1);
    throw_exc = false;
    ep_clnt.remove_watches();
    ep_clnt.register_endpoint(ep_data);
    
    // give a chance for the cleanup to remove the endpoint garbage
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

TEST_F(EndpointClientTest, Register)
{
  endpoint_client ep_clnt(global_mock_ep, "EndpointClientTest");
  endpoint_client ep_clnt2(global_mock_ep, "EndpointClientTest2");
  EXPECT_EQ("EndpointClientTest", ep_clnt.name());
  
  std::atomic<int> nreg{0};
  
  auto ep_callback = [&nreg](const pb::EndpointData & ep)
  {
    if( ep.name() == "EndpointClientTest" &&
        ep.svctype() == pb::ServiceType::OTHER )
    {
      for( auto & conn : ep.connections() )
      {
        if( conn.type() == pb::ConnectionType::REQ_REP )
        {
          for( auto & addr : conn.address() )
          {
            if( addr.find("test-address") == 0 )
            {
              ++nreg;
            }
          }
        }
      }
    }
  };

  std::atomic<int> neps{0};

  auto count_eps = [&neps](const pb::EndpointData & ep)
  {
    if( ep.name() != "config-service" &&
        ep.name() != "ip_discovery" )
    {
      std::cout << "count_eps: " << ep.DebugString() << "\n";
      ++neps;
    }
  };
  
  {
    // check env, count endpoints
    pb::EndpointData ep_data;
    ep_data.set_name("EndpointClientTest-CountEPs");
    ep_data.set_svctype(pb::ServiceType::NONE);
    ep_clnt.register_endpoint(ep_data, count_eps);
    EXPECT_EQ(neps, 0);
    ep_clnt2.register_endpoint(ep_data, count_eps);
    EXPECT_EQ(neps, 0);
    ep_clnt.remove_watches();
    ep_clnt.rethrow_error();
    ep_clnt2.remove_watches();
    ep_clnt2.rethrow_error();
  }
  
  // register a dummy endpoint
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address0");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    ep_clnt.register_endpoint(ep_data, ep_callback);
    EXPECT_GT(nreg, 0);
    nreg = 0;
    ep_clnt.remove_watches();
    ep_clnt.rethrow_error();
    
    // cleanup
    ep_data.set_validforms(1);
    ep_clnt2.register_endpoint(ep_data);
  }
  
  // register another dummy endpoint without a watch
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address1");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    ep_clnt.register_endpoint(ep_data);
    EXPECT_EQ(nreg, 0);
    nreg = 0;
    ep_data.set_validforms(1);
    // same client deregisters
    ep_clnt.register_endpoint(ep_data);
  }
  
  // now register a watch on the other endpoint
  ep_clnt2.watch(pb::ServiceType::OTHER, ep_callback);

  // another register
  {
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address2");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    ep_clnt.register_endpoint(ep_data);
    EXPECT_GT(nreg, 0);
    ep_data.set_validforms(1);
    // other client deregisters
    ep_clnt2.register_endpoint(ep_data);
  }
  
  // giving 100 milliseconds to the config-service to publish the
  // changes and clients to update state
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  {
    // check env, count endpoints
    pb::EndpointData ep_data;
    ep_data.set_name("EndpointClientTest-CountEPs");
    ep_data.set_svctype(pb::ServiceType::NONE);
    ep_clnt.register_endpoint(ep_data, count_eps);
    EXPECT_EQ(neps, 0);
    ep_clnt2.register_endpoint(ep_data, count_eps);
    EXPECT_EQ(neps, 0);
    ep_clnt.remove_watches();
    ep_clnt.rethrow_error();
    ep_clnt2.remove_watches();
    ep_clnt2.rethrow_error();
  }
}



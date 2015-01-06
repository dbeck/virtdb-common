#include "connector_test.hh"
#include <logger.hh>
#include <connector/endpoint_client.hh>
#include <atomic>
#include <thread>

using namespace virtdb::connector;
using namespace virtdb::test;
using namespace virtdb::interface;

extern std::string global_mock_ep;

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
    return true;
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
    return true;
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

TEST_F(EndpointClientTest, Watch) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, Expiry) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, StressRegister) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, StressWatch) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, MonitorException) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, InvalidConstr) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, InvalidWatch) { EXPECT_TRUE(false); }
TEST_F(EndpointClientTest, InvalidRegister) { EXPECT_TRUE(false); }


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

TEST_F(ConfigClientTest, ImplementMe) { EXPECT_TRUE(false); }


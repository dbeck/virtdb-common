#include "connector_test.hh"
#include <logger.hh>
#include <connector/server_base.hh>
#include <connector/server_context.hh>
#include <connector/client_context.hh>
#include <connector/endpoint_client.hh>
#include <connector/config_client.hh>
#include <connector/cert_store_client.hh>
#include <connector/srcsys_credential_client.hh>
#include <connector/ip_discovery_client.hh>
#include <connector/monitoring_server.hh>
#include <connector/monitoring_client.hh>
#include <test/cfgsvc_mock.hh>
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

void ConnectorCommon::SetUp()
{
  cctx_.reset(new client_context);
}

TEST_F(ConnMonitoringTest, SimpleConnect)
{
  const char * name = "ConnMonitoringTest-SimpleConnect";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  monitoring_client cli{cctx_, ep_clnt, "monitoring-service"};
  EXPECT_TRUE(cli.wait_valid(100));
}

TEST_F(ConnMonitoringTest, ReportStats)
{
  const char * name = "ConnMonitoringTest-ReportStats";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  monitoring_client cli{cctx_, ep_clnt, "monitoring-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  const char * keys[] = { "Hello", "World", NULL };
  EXPECT_TRUE(report_stats(cli,  name, keys));
  EXPECT_FALSE(report_stats(cli,  name, NULL));
  
  pb::MonitoringRequest req;
  req.set_type(pb::MonitoringRequest::GET_STATES);
  req.mutable_getsts();
  EXPECT_TRUE(cli.send_request(req, [](const pb::MonitoringReply & rep)
  {
    return true;
  },10000));
}

TEST_F(ConnMonitoringTest, SetStates)
{
  const char * name = "ConnMonitoringTest-SetStates";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  monitoring_client cli{cctx_, ep_clnt, "monitoring-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  
  EXPECT_TRUE(send_state(cli, name, false));
  EXPECT_FALSE(check_ok(cli, name));
  EXPECT_TRUE(send_state(cli, name, true));
  EXPECT_TRUE(check_ok(cli, name));
}

TEST_F(ConnMonitoringTest, ComponentError)
{
  const char * name = "ConnMonitoringTest-ComponentError";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  monitoring_client cli{cctx_, ep_clnt, "monitoring-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  
  EXPECT_TRUE(send_comperr(cli, "apple", name, false));
  EXPECT_FALSE(check_ok(cli, name));
  EXPECT_TRUE(send_comperr(cli, "peach", name, false));
  EXPECT_FALSE(check_ok(cli, name));
  EXPECT_TRUE(send_comperr(cli, "apple", name, true));
  EXPECT_FALSE(check_ok(cli, name));
  EXPECT_TRUE(send_comperr(cli, "peach", name, true));
  EXPECT_TRUE(check_ok(cli, name));
}

bool
ConnMonitoringTest::send_comperr(connector::monitoring_client & cli,
                                const std::string & reporting,
                                 const std::string & impacted,
                                 bool clear)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::MonitoringRequest req;
  req.set_type(pb::MonitoringRequest::COMPONENT_ERROR);
  auto stats = req.mutable_comperr();
  stats->set_reportedby(reporting);
  stats->set_impactedpeer(impacted);
  stats->set_type(clear?(pb::MonitoringRequest::ComponentError::CLEAR):(pb::MonitoringRequest::ComponentError::UPSTREAM_ERROR));
  pb::MonitoringReply rep;
  
  auto proc_rep = [&](const pb::MonitoringReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
    return false;
  EXPECT_EQ(rep.type(), pb::MonitoringReply::COMPONENT_ERROR);
  return !rep.has_err();
}


bool
ConnMonitoringTest::send_state(connector::monitoring_client & cli,
                             const std::string & name,
                             bool clear)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::MonitoringRequest req;
  req.set_type(pb::MonitoringRequest::SET_STATE);
  auto stats = req.mutable_setst();
  stats->set_name(name);
  stats->set_type(clear?(pb::MonitoringRequest::SetState::CLEAR):(pb::MonitoringRequest::SetState::COMPONENT_ERROR));
  pb::MonitoringReply rep;
  
  auto proc_rep = [&](const pb::MonitoringReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
    return false;
  EXPECT_EQ(rep.type(), pb::MonitoringReply::SET_STATE);
  return !rep.has_err();
}

bool
ConnMonitoringTest::check_ok(connector::monitoring_client & cli,
                             const std::string & name)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::MonitoringRequest req;
  req.set_type(pb::MonitoringRequest::GET_STATES);
  auto stats = req.mutable_getsts();
  stats->set_name(name);
  
  pb::MonitoringReply rep;
  
  auto proc_rep = [&](const pb::MonitoringReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
    return false;
  EXPECT_EQ(rep.type(), pb::MonitoringReply::GET_STATES);
  if(!rep.has_states())
    return false;
  auto const & states = rep.states();
  if( states.states_size() == 1 )
  {
    EXPECT_EQ(states.states(0).name(), name);
    LOG_TRACE(M_(rep));
    return states.states(0).ok();
  }
  return false;
}

bool
ConnMonitoringTest::report_stats(connector::monitoring_client & cli,
                                 const std::string & name,
                                 const char ** keys)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::MonitoringRequest req;
  req.set_type(pb::MonitoringRequest::REPORT_STATS);
  auto stats = req.mutable_repstats();
  stats->set_name(name);
  if( keys )
  {
    const char * k = keys[0];
    while( k )
    {
      auto st = stats->add_stats();
      st->set_name(k);
      st->set_stat(1.1);
      k = *(++keys);
    }
  }
  
  pb::MonitoringReply rep;
  
  auto proc_rep = [&](const pb::MonitoringReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
    return false;
  EXPECT_EQ(rep.type(), pb::MonitoringReply::REPORT_STATS);
  return !rep.has_err();
}


TEST_F(ConnEndpointTest, StressWatch)
{
  const char * name = "EndpointClientTest-StressWatch";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  
  EXPECT_TRUE(ep_clnt.wait_valid_req(1000));
  EXPECT_TRUE(ep_clnt.wait_valid_sub(1000));
  
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
    ep_data.set_cmd(pb::EndpointData::ADD);
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

TEST_F(ConnEndpointTest, StressRegister)
{
  const char * name = "EndpointClientTest-StressRegister";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  pb::EndpointData ep_data;
  
  EXPECT_TRUE(ep_clnt.wait_valid_req(1000));
  EXPECT_TRUE(ep_clnt.wait_valid_sub(1000));
  
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

TEST_F(ConnEndpointTest, InvalidRegister)
{
  const char * name = "EndpointClientTest-InvalidRegister";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  
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

TEST_F(ConnEndpointTest, InvalidWatch)
{
  const char * name = "EndpointClientTest-InvalidWatch";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
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

TEST_F(ConnEndpointTest, InvalidConstr)
{
  auto invalid_svc_config_ep = [&]() {
    endpoint_client ep_clnt(cctx_, "", "EndpointClientTest-InvalidConstr");
  };
  EXPECT_THROW(invalid_svc_config_ep(), virtdb::util::exception);
  
  auto invalid_service_name = [&]() {
    endpoint_client ep_clnt(cctx_, "tcp://0.0.0.0:0", "");
  };
  EXPECT_THROW(invalid_service_name(), virtdb::util::exception);
  
  auto malformed_ep = [&]() {
    endpoint_client ep_clnt(cctx_, "invalid ep", "xxx");
  };
  EXPECT_THROW(malformed_ep(), virtdb::util::exception);
}

TEST_F(ConnEndpointTest, Watch)
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
  
  endpoint_client ep_clnt(cctx_, global_mock_ep, "EndpointClientTest-Watch");
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

TEST_F(ConnEndpointTest, Expiry)
{
  const char * name = "EndpointClientTest-Expiry";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  
  EXPECT_TRUE(ep_clnt.wait_valid_req(1000));
  EXPECT_TRUE(ep_clnt.wait_valid_sub(1000));
  
  
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
    std::this_thread::sleep_for(std::chrono::milliseconds{400});
    
    // check endpoints
    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::NONE);
    
    std::atomic_int counter{0};
    ep_clnt.register_endpoint(ep_data,[name,&counter](const pb::EndpointData & ep)
                              {
                                // make sure we don't get back the expired EP data
                                EXPECT_NE(ep.name(), name);
                                if( ep.name() == name )
                                  ++counter;
                              });
    EXPECT_EQ(counter, 0);
  }
}

TEST_F(ConnEndpointTest, MonitorException)
{
  const char * name = "EndpointClientTest-MonitorException";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  
  EXPECT_TRUE(ep_clnt.wait_valid_req(1000));
  EXPECT_TRUE(ep_clnt.wait_valid_sub(1000));
  
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

TEST_F(ConnEndpointTest, Register)
{
  endpoint_client ep_clnt(cctx_, global_mock_ep, "EndpointClientTest");
  endpoint_client ep_clnt2(cctx_, global_mock_ep, "EndpointClientTest2");
  EXPECT_EQ("EndpointClientTest", ep_clnt.name());
  
  std::atomic<int> nreg{0};
  std::shared_ptr<std::promise<void>> prom_ptr{new std::promise<void>()};
  std::shared_ptr<std::future<void>> fut_ptr{new std::future<void>{prom_ptr->get_future()}};
  
  auto ep_callback = [&nreg, &prom_ptr](const pb::EndpointData & ep)
  {
    if( ep.name() == "EndpointClientTest" &&
        ep.svctype() == pb::ServiceType::OTHER )
    {
      std::cout << "\nEP CB: " << ep.DebugString() << "\n";
      for( auto & conn : ep.connections() )
      {
        if( conn.type() == pb::ConnectionType::REQ_REP )
        {
          for( auto & addr : conn.address() )
          {
            if( addr.find("test-address") == 0 )
            {
              ++nreg;
              if( nreg == 1 )
                prom_ptr->set_value();
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
        ep.name() != "ip_discovery" &&
        ep.name() != "security-service" &&
        ep.name() != "monitoring-service" )
    {
      std::cout << "count_eps: " << ep.DebugString() << "\n";
      ++neps;
    }
  };
  
  {
    nreg = 0;
    prom_ptr.reset(new std::promise<void>());
    fut_ptr.reset(new std::future<void>{prom_ptr->get_future()});

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
    nreg = 0;
    prom_ptr.reset(new std::promise<void>());
    fut_ptr.reset(new std::future<void>{prom_ptr->get_future()});

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
    nreg = 0;
    prom_ptr.reset(new std::promise<void>());
    fut_ptr.reset(new std::future<void>{prom_ptr->get_future()});

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
    nreg = 0;
    prom_ptr.reset(new std::promise<void>());
    fut_ptr.reset(new std::future<void>{prom_ptr->get_future()});

    pb::EndpointData ep_data;
    ep_data.set_name(ep_clnt.name());
    ep_data.set_svctype(pb::ServiceType::OTHER);
    
    auto conn = ep_data.add_connections();
    conn->add_address("test-address2");
    conn->set_type(pb::ConnectionType::REQ_REP);
    
    ep_clnt.register_endpoint(ep_data);
    EXPECT_EQ(fut_ptr->wait_for(std::chrono::seconds(10)),std::future_status::ready);
    EXPECT_GT(nreg, 0);
    ep_data.set_validforms(1);
    // other client deregisters
    ep_clnt2.register_endpoint(ep_data);
  }
  
  // giving 100 milliseconds to the config-service to publish the
  // changes and clients to update state
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  {
    nreg = 0;
    prom_ptr.reset(new std::promise<void>());
    fut_ptr.reset(new std::future<void>{prom_ptr->get_future()});

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

TEST_F(ConnConfigTest, RemoveNonexistentWatches)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-RemoveNonexistentWatches";
  auto test_fun = [&]() {
    endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
    config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
    
    // this is OK:
    cfg_clnt.remove_watches();
  };
  
  EXPECT_NO_THROW(test_fun());
}

TEST_F(ConnConfigTest, DoubleCleanupRethrow)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-DoubleCleanupRethrow";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  // this sequence is valid, rethrow should throw excpetions here
  cfg_clnt.cleanup();
  cfg_clnt.rethrow_error();
  cfg_clnt.cleanup();
  cfg_clnt.rethrow_error();
}

TEST_F(ConnConfigTest, TripleRethrow)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-TripleRethrow";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  // rethrow should survive this without exception or crash
  cfg_clnt.rethrow_error();
  cfg_clnt.rethrow_error();
  cfg_clnt.rethrow_error();
}

TEST_F(ConnConfigTest, TripleCleanup)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-TripleCleanup";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  // cleanup should survive this without exception or crash
  cfg_clnt.cleanup();
  cfg_clnt.cleanup();
  cfg_clnt.cleanup();
}

TEST_F(ConnConfigTest, BadServiceNameToConnect)
{
  // this should succeed without exception and error
  const char * name = "ConfigClientTest-BadServiceNameToConnect";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "nope-config-service");
  
  // this should fail because no such service exists
  EXPECT_FALSE(cfg_clnt.wait_valid_sub(200));
  EXPECT_FALSE(cfg_clnt.wait_valid_req(200));
}

TEST_F(ConnConfigTest, SimpleConnect)
{
  // only test that it doesn't fail with an error or an exception
  const char * name = "ConfigClientTest-ConnectOnly";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  // this should succeed assuming the mock service is running
  EXPECT_TRUE(cfg_clnt.wait_valid_sub(200));
  EXPECT_TRUE(cfg_clnt.wait_valid_req(200));
}

TEST_F(ConnConfigTest, CheckReqChannel)
{
  const char * name = "ConfigClientTest-CheckReqChannel";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  {
    pb::Config cfg_req, cfg_rep;
    cfg_req.set_name(name);
    
    auto resp = [&cfg_rep](const pb::Config & rep) {
      cfg_rep.MergeFrom(rep);
      return true;
    };
    
    EXPECT_TRUE(cfg_clnt.send_request(cfg_req, resp, 10000));
    EXPECT_EQ(cfg_rep.name(), name);
  }
}

TEST_F(ConnConfigTest, CheckSubChannel)
{
  const char * name = "ConfigClientTest-CheckSubChannel";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  std::shared_ptr<pb::Config> cfg_sub_res;
  std::promise<void> cfg_promise;
  std::future<void> on_cfg{cfg_promise.get_future()};
  
  
  EXPECT_TRUE(cfg_clnt.wait_valid_sub(1000));
  
  auto config_watch = [&](const std::string & provider_name,
                          const std::string & channel,
                          const std::string & subscription,
                          std::shared_ptr<pb::Config> cfg)
  {
    EXPECT_EQ(cfg->name(),name);
    if( cfg->name() == name )
    {
      cfg_sub_res = cfg;
      cfg_promise.set_value();
    }
  };
  
  // set monitor
  cfg_clnt.watch(name, config_watch);
  
  {
    pb::Config cfg_req, cfg_rep;
    cfg_req.set_name(name);
    auto * dta = cfg_req.mutable_configdata();
    auto * kv = dta->Add();
    kv->set_key("");
    
    {
      auto child = kv->add_children();
      child->set_key("Config Parameter");
      {
        auto child_l2 = child->add_children();
        child_l2->set_key("Value");
        auto value_l2 = child_l2->mutable_value();
        value_l2->set_type(pb::Kind::STRING);
      }
      {
        auto child_l2 = child->add_children();
        child_l2->set_key("Scope");
        auto value_l2 = child_l2->mutable_value();
        value_l2->add_stringvalue("user_config");
        value_l2->set_type(pb::Kind::STRING);
      }
    }
    
    auto resp = [&cfg_rep](const pb::Config & rep) {
      cfg_rep.MergeFrom(rep);
      return true;
    };
    
    EXPECT_TRUE(cfg_clnt.send_request(cfg_req, resp, 10000));
    EXPECT_EQ(cfg_rep.name(), name);
  }
  
  EXPECT_EQ(on_cfg.wait_for(std::chrono::seconds(10)), std::future_status::ready);
}

TEST_F(ConnServerBaseTest, ConstuctHostSet)
{
  const char * name = "ServerBaseTest-ConstuctHostSet";
  endpoint_client   ep_clnt(cctx_, global_mock_ep, name);
  config_client     cfg_clnt(cctx_, ep_clnt, "config-service");
  
  server_context::sptr ctx{new server_context};
  ctx->service_name("dummy-service");
  ctx->endpoint_svc_addr(global_mock_ep);
  ctx->ip_discovery_timeout_ms(10);
  server_base bs{ctx};
  auto const & host_set{bs.hosts(ep_clnt)};
}

bool
ConnCertStoreTest::create_temp_key(connector::cert_store_client & cli,
                                   const std::string & name,
                                   std::string & authcode)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  authcode.clear();
  
  pb::Certificate      crt;
  crt.set_componentname(name);
  crt.set_publickey("public-key");
  crt.set_approved(false);
  
  pb::CertStoreRequest req;
  req.set_type(pb::CertStoreRequest::CREATE_TEMP_KEY);
  req.mutable_create()->mutable_cert()->MergeFrom(crt);
  pb::CertStoreReply   rep;
  
  auto proc_rep = [&](const pb::CertStoreReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::CertStoreReply::CREATE_TEMP_KEY);
  EXPECT_TRUE(rep.has_create());
  EXPECT_TRUE(rep.create().has_authcode());
  if( rep.create().has_authcode() )
  {
    authcode = rep.create().authcode();
  }

  return !rep.has_err();
}

bool
ConnCertStoreTest::approve_temp_key(connector::cert_store_client & cli,
                                    const std::string & name,
                                    const std::string & authcode)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::CertStoreRequest req;
  {
    req.set_type(pb::CertStoreRequest::APPROVE_TEMP_KEY);
    auto * inner = req.mutable_approve();
    inner->set_authcode(authcode);
    inner->set_logintoken("token");
    inner->set_componentname(name);
  }
  pb::CertStoreReply   rep;
  
  auto proc_rep = [&](const pb::CertStoreReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
  {
    return false;
  }
  EXPECT_EQ(rep.type(), pb::CertStoreReply::APPROVE_TEMP_KEY);
  return !rep.has_err();
}

bool
ConnCertStoreTest::has_temp_key(connector::cert_store_client & cli,
                                const std::string & name)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::CertStoreRequest req;
  {
    req.set_type(pb::CertStoreRequest::LIST_KEYS);
    auto * inner = req.mutable_list();
    inner->set_tempkeys(true);
    inner->set_approvedkeys(false);
  }
  pb::CertStoreReply   rep;
  
  auto proc_rep = [&](const pb::CertStoreReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::CertStoreReply::LIST_KEYS);
  EXPECT_TRUE(rep.has_list());
  EXPECT_FALSE(rep.has_err());
  
  if( rep.has_err() ) return false;
  
  bool ret = false;
  for( auto const & c : rep.list().certs() )
  {
    if( c.componentname() == name )
      return true;
  }
  return false;
}

bool
ConnCertStoreTest::has_approved_key(connector::cert_store_client & cli,
                                    const std::string & name)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::CertStoreRequest req;
  {
    req.set_type(pb::CertStoreRequest::LIST_KEYS);
    auto * inner = req.mutable_list();
    inner->set_tempkeys(false);
    inner->set_approvedkeys(true);
  }
  pb::CertStoreReply   rep;
  
  auto proc_rep = [&](const pb::CertStoreReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::CertStoreReply::LIST_KEYS);
  EXPECT_TRUE(rep.has_list());
  EXPECT_FALSE(rep.has_err());
  
  if( rep.has_err() )
    return false;
  
  bool ret = false;
  for( auto const & c : rep.list().certs() )
  {
    if( c.componentname() == name )
      return true;
  }
  return false;
}

bool
ConnCertStoreTest::has_key(connector::cert_store_client & cli,
                           const std::string & name)
{
  bool t2 = false;
  bool t3 = false;
  int sz = 0;
  
  { // t2
    std::promise<void> rep_promise;
    std::future<void>  on_rep{rep_promise.get_future()};
    
    pb::CertStoreRequest req;
    {
      req.set_type(pb::CertStoreRequest::LIST_KEYS);
      auto * inner = req.mutable_list();
      inner->set_tempkeys(true);
      inner->set_approvedkeys(false);
      inner->set_componentname(name);
    }
    pb::CertStoreReply   rep;
    
    auto proc_rep = [&](const pb::CertStoreReply & r) {
      rep.MergeFrom(r);
      rep_promise.set_value();
      return true;
    };
    
    EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
    EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_EQ(rep.type(), pb::CertStoreReply::LIST_KEYS);
    EXPECT_TRUE(rep.has_list());
    EXPECT_FALSE(rep.has_err());
    sz += rep.list().certs_size();
    
    for( auto const & c : rep.list().certs() )
    {
      if( c.componentname() == name )
        t2 = true;
    }
    
    if( rep.has_err() )
      t2 = false;
  }
  
  { // t3
    std::promise<void> rep_promise;
    std::future<void>  on_rep{rep_promise.get_future()};
    
    pb::CertStoreRequest req;
    {
      req.set_type(pb::CertStoreRequest::LIST_KEYS);
      auto * inner = req.mutable_list();
      inner->set_tempkeys(false);
      inner->set_approvedkeys(true);
      inner->set_componentname(name);
    }
    pb::CertStoreReply   rep;
    
    auto proc_rep = [&](const pb::CertStoreReply & r) {
      rep.MergeFrom(r);
      rep_promise.set_value();
      return true;
    };
    
    EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
    EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_EQ(rep.type(), pb::CertStoreReply::LIST_KEYS);
    EXPECT_TRUE(rep.has_list());
    EXPECT_FALSE(rep.has_err());
    sz += rep.list().certs_size();
    
    for( auto const & c : rep.list().certs() )
    {
      if( c.componentname() == name )
        t3 = true;
    }
    
    if( rep.has_err() )
      t3 = false;
  }
  
  return ((t2 || t3) && sz>0);
}

bool
ConnCertStoreTest::delete_key(connector::cert_store_client & cli,
                              const std::string & name)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::Certificate      crt;
  crt.set_componentname(name);
  crt.set_publickey("public-key");
  crt.set_approved(false);
  
  pb::CertStoreRequest req;
  {
    req.set_type(pb::CertStoreRequest::DELETE_KEY);
    auto * inner = req.mutable_del();
    inner->mutable_cert()->MergeFrom(crt);
    inner->set_logintoken("none");
  }
  pb::CertStoreReply   rep;
  
  auto proc_rep = [&](const pb::CertStoreReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::CertStoreReply::DELETE_KEY);
  
  return !rep.has_err();
}

TEST_F(ConnCertStoreTest, SimpleConnect)
{
  const char * name = "ConnCertStoreTest-SimpleConnect";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  cert_store_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
}

TEST_F(ConnCertStoreTest, CreateTempKey)
{
  const char * name = "ConnCertStoreTest-CreateTempKey";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  cert_store_client cli{cctx_, ep_clnt, "security-service"};
  
  EXPECT_TRUE(cli.wait_valid(100));
  std::string authcode;
  EXPECT_TRUE(create_temp_key(cli, name, authcode));
  EXPECT_FALSE(authcode.empty());
}

TEST_F(ConnCertStoreTest, ApproveTempKey)
{
  const char * name = "ConnCertStoreTest-ApproveTempKey";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  cert_store_client cli{cctx_, ep_clnt, "security-service"};
  
  EXPECT_TRUE(cli.wait_valid(100));
  std::string authcode{"code"};
  EXPECT_FALSE(approve_temp_key(cli, name, authcode));
  EXPECT_TRUE(create_temp_key(cli, name, authcode));
  EXPECT_FALSE(authcode.empty());
  EXPECT_NE(authcode,"code");
  EXPECT_TRUE(approve_temp_key(cli, name, authcode));
  
  // no double auth
  EXPECT_FALSE(approve_temp_key(cli, name, authcode));
  EXPECT_TRUE(delete_key(cli, name));
  
  // approve after deleted
  EXPECT_TRUE(create_temp_key(cli, name, authcode));
  EXPECT_TRUE(delete_key(cli, name));
  EXPECT_FALSE(approve_temp_key(cli, name, authcode));
}

TEST_F(ConnCertStoreTest, ListKeys)
{
  const char * name = "ConnCertStoreTest-ListKeys";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  cert_store_client cli{cctx_, ep_clnt, "security-service"};

  EXPECT_TRUE(cli.wait_valid(100));
  std::string authcode{"code"};
  EXPECT_TRUE(create_temp_key(cli, name, authcode));
  EXPECT_TRUE(has_temp_key(cli, name));
  EXPECT_FALSE(has_approved_key(cli, name));
  EXPECT_TRUE(has_key(cli, name));
  EXPECT_TRUE(approve_temp_key(cli, name, authcode));
  EXPECT_TRUE(has_approved_key(cli, name));
  EXPECT_TRUE(has_key(cli, name));
  EXPECT_FALSE(has_temp_key(cli, name));

  EXPECT_TRUE(delete_key(cli, name));
  EXPECT_FALSE(has_key(cli, name));
  EXPECT_FALSE(has_temp_key(cli, name));
  EXPECT_FALSE(has_approved_key(cli, name));
}

TEST_F(ConnCertStoreTest, DeleteKey)
{
  const char * name = "ConnCertStoreTest-DeleteKey";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  cert_store_client cli{cctx_, ep_clnt, "security-service"};

  EXPECT_TRUE(cli.wait_valid(100));
  std::string authcode{"code"};
  EXPECT_TRUE(create_temp_key(cli, name, authcode));
  EXPECT_TRUE(has_key(cli, name));
  EXPECT_TRUE(delete_key(cli, name));
  EXPECT_FALSE(has_key(cli, name));
  EXPECT_FALSE(has_temp_key(cli, name));
  EXPECT_FALSE(has_approved_key(cli, name));
}

bool
ConnSrcsysCredTest::set_cred(connector::srcsys_credential_client & cli,
                             const std::string & srcsys,
                             const std::string & token)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::SourceSystemCredentialRequest req;
  {
    req.set_type(pb::SourceSystemCredentialRequest::SET_CREDENTIAL);
    auto * inner = req.mutable_setcred();
    inner->set_sourcesysname(srcsys);
    inner->set_sourcesystoken(token);
    auto * crds = inner->mutable_creds();
    auto * nv = crds->add_namedvalues();
    nv->set_name("name");
    nv->set_value("value");
  }
  pb::SourceSystemCredentialReply rep;
  
  auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::SourceSystemCredentialReply::SET_CREDENTIAL);
  return !rep.has_err();
}

bool
ConnSrcsysCredTest::get_cred(connector::srcsys_credential_client & cli,
                             const std::string & srcsys,
                             const std::string & token)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::SourceSystemCredentialRequest req;
  {
    req.set_type(pb::SourceSystemCredentialRequest::GET_CREDENTIAL);
    auto * inner = req.mutable_getcred();
    inner->set_sourcesysname(srcsys);
    inner->set_sourcesystoken(token);
  }
  pb::SourceSystemCredentialReply rep;
  
  auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() ) return false;
  EXPECT_EQ(rep.type(), pb::SourceSystemCredentialReply::GET_CREDENTIAL);
  EXPECT_TRUE(rep.has_getcred());
  if( !rep.getcred().has_creds() ) return false;
  EXPECT_EQ(rep.getcred().creds().namedvalues_size(), 1);
  return !rep.has_err();
}

bool
ConnSrcsysCredTest::del_cred(connector::srcsys_credential_client & cli,
                             const std::string & srcsys,
                             const std::string & token)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::SourceSystemCredentialRequest req;
  {
    req.set_type(pb::SourceSystemCredentialRequest::DELETE_CREDENTIAL);
    auto * inner = req.mutable_delcred();
    inner->set_sourcesysname(srcsys);
    inner->set_sourcesystoken(token);
  }
  pb::SourceSystemCredentialReply rep;
  
  auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() ) return false;
  EXPECT_EQ(rep.type(), pb::SourceSystemCredentialReply::DELETE_CREDENTIAL);
  return !rep.has_err();
}

bool
ConnSrcsysCredTest::set_tmpl(connector::srcsys_credential_client & cli,
                             const std::string & srcsys)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::SourceSystemCredentialRequest req;
  {
    req.set_type(pb::SourceSystemCredentialRequest::SET_TEMPLATE);
    auto * inner = req.mutable_settmpl();
    inner->set_sourcesysname(srcsys);
    auto * tmpl = inner->add_templates();
    tmpl->set_name("name");
    tmpl->set_type(pb::FieldTemplate::STRING);
  }
  pb::SourceSystemCredentialReply rep;
  
  auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(rep.type(), pb::SourceSystemCredentialReply::SET_TEMPLATE);
  return !rep.has_err();
}

bool
ConnSrcsysCredTest::get_tmpl(connector::srcsys_credential_client & cli,
                             const std::string & srcsys)
{
  std::promise<void> rep_promise;
  std::future<void>  on_rep{rep_promise.get_future()};
  
  pb::SourceSystemCredentialRequest req;
  {
    req.set_type(pb::SourceSystemCredentialRequest::GET_TEMPLATE);
    auto * inner = req.mutable_gettmpl();
    inner->set_sourcesysname(srcsys);
  }
  pb::SourceSystemCredentialReply rep;
  
  auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
    rep.MergeFrom(r);
    rep_promise.set_value();
    return true;
  };
  
  EXPECT_TRUE(cli.send_request(req, proc_rep, 10000));
  EXPECT_EQ(on_rep.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  if( rep.has_err() )
    return false;
  EXPECT_EQ(rep.type(), pb::SourceSystemCredentialReply::GET_TEMPLATE);
  EXPECT_TRUE(rep.has_gettmpl());
  EXPECT_NE(rep.gettmpl().templates_size(), 0);
  return !rep.has_err();
}


TEST_F(ConnSrcsysCredTest, SimpleConnect)
{
  const char * name = "ConnSrcsysCredTest-SimpleConnect";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
}

TEST_F(ConnSrcsysCredTest, SetCredential)
{
  const char * name = "ConnSrcsysCredTest-SetCredential";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  EXPECT_TRUE(set_cred(cli, name, "token"));
  EXPECT_TRUE(get_cred(cli, name, "token"));
  EXPECT_TRUE(set_cred(cli, name, "token"));
  EXPECT_TRUE(set_cred(cli, name, "token"));
}

TEST_F(ConnSrcsysCredTest, GetCredential)
{
  const char * name = "ConnSrcsysCredTest-GetCredential";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  EXPECT_FALSE(get_cred(cli, name, "bad-token"));
  EXPECT_FALSE(get_cred(cli, name, "token"));
  EXPECT_TRUE(set_cred(cli, name, "token"));
  EXPECT_TRUE(get_cred(cli, name, "token"));
  EXPECT_FALSE(get_cred(cli, name, "bad-token"));
}

TEST_F(ConnSrcsysCredTest, DeleteCredential)
{
  const char * name = "ConnSrcsysCredTest-DeleteCredential";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  EXPECT_FALSE(del_cred(cli, name, "token"));
  EXPECT_TRUE(set_cred(cli, name, "token"));
  EXPECT_TRUE(del_cred(cli, name, "token"));
  EXPECT_FALSE(del_cred(cli, name, "token"));
}

TEST_F(ConnSrcsysCredTest, SetTemplate)
{
  const char * name = "ConnSrcsysCredTest-SetTemplate";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  EXPECT_TRUE(set_tmpl(cli, name));
  EXPECT_TRUE(get_tmpl(cli, name));
  EXPECT_TRUE(set_tmpl(cli, name));
  EXPECT_TRUE(set_tmpl(cli, name));
}

TEST_F(ConnSrcsysCredTest, GetTemplate)
{
  const char * name = "ConnSrcsysCredTest-GetTemplate";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  srcsys_credential_client cli{cctx_, ep_clnt, "security-service"};
  EXPECT_TRUE(cli.wait_valid(100));
  EXPECT_FALSE(get_tmpl(cli, name));
  EXPECT_TRUE(set_tmpl(cli, name));
  EXPECT_TRUE(get_tmpl(cli, name));
}

/*
SET_CREDENTIAL       = 2;
GET_CREDENTIAL       = 3;
DELETE_CREDENTIAL    = 4;
SET_TEMPLATE         = 5;
GET_TEMPLATE         = 6;
*/

TEST_F(ConnIpDiscoveryTest, SimpleConnect)
{
  const char * name = "ConnIpDiscoveryTest-SimpleConnect";
  endpoint_client ep_clnt(cctx_, global_mock_ep, name);
  auto res = ip_discovery_client::get_ip(ep_clnt);
  EXPECT_FALSE(res.empty());
}

/*
TEST_F(ConnLogRecordTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnQueryTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnColumnTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnMetaDataTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnPubSubTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnPushPullTest, ImplementMe) { EXPECT_TRUE(false); }
TEST_F(ConnReqRepTest, ImplementMe) { EXPECT_TRUE(false); }
*/


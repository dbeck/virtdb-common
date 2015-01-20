
#include <logger.hh>
#include <fault/injector.hh>
// #include <connector.hh>
#include <connector/endpoint_server.hh>
#include <connector/endpoint_client.hh>
#include <connector/config_server.hh>
#include <connector/config_client.hh>
#include <iostream>

using namespace virtdb::connector;
using namespace virtdb::logger;
using namespace virtdb::interface;

namespace
{
  template <typename EXC>
  int usage(const EXC & exc)
  {
    std::cerr << "Exception: " << exc.what() << "\n"
              << "\n"
#ifdef INJECT_FAULTS
              << "Usage: cfgsvc_mock_with_faults <Request-Reply-EndPoint>\n"
#else
              << "Usage: cfgsvc_mock <Request-Reply-EndPoint>\n"
#endif
              << "\n";
    return 100;
  }
}

int main(int argc, char ** argv)
{
  using virtdb::logger::log_sink;
  
  try
  {
    if( argc < 2 )
    {
      THROW_("invalid number of arguments");
    }
    
    bool stop = false;
    if( argc > 2 && std::string{"stop"} == argv[2] )
    {
      stop = true;
    }
    
    std::string service_ep{argv[1]};
    
    if( !stop )
    {
      log_sink::socket_sptr  dummy_socket;
      log_sink::sptr         sink_stderr{new log_sink{dummy_socket}};
    
      LOG_SCOPED("end server block");
      {
        endpoint_server     ep_srv(service_ep, "config-service");
        endpoint_client     ep_clnt(ep_srv.local_ep(), ep_srv.name());
        config_client       cfg_clnt(ep_clnt, "config-service");
        config_server       cfg_srv(cfg_clnt, ep_srv);
        
        std::atomic<int> i{0};
        
        // stop on OTHER:STOP endpoint
        ep_clnt.watch(pb::ServiceType::OTHER,
                      [&i](const pb::EndpointData & ep)
                      {
                        LOG_TRACE("received message" << V_(ep.name()) << M_(ep));
                        if( ep.name() == "STOP" )
                        {
                          i += 10000;
                        }
                      });
        
        for( ; i<20*60; ++i )
        {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        LOG_TRACE("stopping mock service");
      }
    }
    else
    {
      endpoint_client ep_clnt(service_ep,  "STOP");
      
      pb::EndpointData ep_data;
      {
        ep_data.set_name("STOP");
        ep_data.set_svctype(pb::ServiceType::OTHER);
        
        auto conn = ep_data.add_connections();
        conn->add_address("invalid address");
        conn->set_type(pb::ConnectionType::REQ_REP);
        
        ep_clnt.register_endpoint(ep_data);
      }

    }
  }
  catch (const std::exception & e)
  {
    return usage(e);
  }
  return 0;
}

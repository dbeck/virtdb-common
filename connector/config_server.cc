#include "config_server.hh"
#include <logger.hh>
#include <util/net.hh>

using namespace virtdb;
using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {

  config_server::config_server(config_client & cfg_client,
                               endpoint_server & ep_server)
  : zmqctx_(1),
    cfg_pull_socket_(zmqctx_, ZMQ_PULL),
    cfg_pub_socket_(zmqctx_, ZMQ_PUB),
    worker_(std::bind(&config_server::worker_function,this))
  {
    // collect hosts to bind to
    zmq_socket_wrapper::host_set hosts;
    {
      // add my ips
      net::string_vector my_ips{net::get_own_ips(true)};
      hosts.insert(my_ips.begin(), my_ips.end());

      // add endpoint_server hostnames too
      auto ep_global = parse_zmq_tcp_endpoint(ep_server.global_ep());
      auto ep_local = parse_zmq_tcp_endpoint(ep_server.local_ep());
      hosts.insert(ep_global.first);
      hosts.insert(ep_local.first);
      hosts.insert("*");
    }
    
    cfg_pull_socket_.batch_tcp_bind(hosts);
    cfg_pub_socket_.batch_tcp_bind(hosts);
    
    // setting up our own endpoints
    {
      pb::EndpointData ep_data;
      {
        ep_data.set_name(ep_server.name());
        ep_data.set_svctype(pb::ServiceType::CONFIG);
        
        {
          // PULL socket
          auto conn = ep_data.add_connections();
          conn->set_type(pb::ConnectionType::PUSH_PULL);
          
          for( auto const & ep : cfg_pull_socket_.endpoints() )
            *(conn->add_address()) = ep;
        }
     
        {
          // PUB socket
          auto conn = ep_data.add_connections();
          conn->set_type(pb::ConnectionType::PUB_SUB);
          
          for( auto const & ep : cfg_pub_socket_.endpoints() )
            *(conn->add_address()) = ep;
        }
        
        cfg_client.get_endpoint_client().register_endpoint(ep_data);
      }
    }
    
    worker_.start();
  }
  
  config_server::~config_server()
  {
    worker_.stop();
  }
  
  bool
  config_server::worker_function()
  {
    zmq::pollitem_t poll_item{ cfg_pull_socket_.get(), 0, ZMQ_POLLIN, 0 };
    if( zmq::poll(&poll_item, 1, 3000) == -1 ||
       !(poll_item.revents & ZMQ_POLLIN) )
    {
      return true;
    }

    zmq::message_t message;
    if( !cfg_pull_socket_.get().recv(&message) )
      return true;
    
    pb::Config request;
    if( !message.data() || !message.size())
      return true;

    // TODO : read pull socket and publish
    LOG_ERROR("TODO : implement me");

    
    return true;
  }
}}

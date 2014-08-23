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
    }
    
    cfg_pull_socket_.batch_tcp_bind(hosts);
    cfg_pub_socket_.batch_tcp_bind(hosts);
   
    // TODO : initialize

    
    worker_.start();
  }
  
  config_server::~config_server()
  {
    worker_.stop();
  }
  
  bool
  config_server::worker_function()
  {
    LOG_ERROR("TODO : implement me");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
  }
}}

#include "server_base.hh"
#include "config_client.hh"
#include "ip_discovery_client.hh"
#include <util/net.hh>

using namespace virtdb;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  server_base::server_base(config_client & cfg_client)
  : name_(cfg_client.get_endpoint_client().name())
  {
    // collect hosts to bind to
    zmq_socket_wrapper::host_set hosts;
    {
      // add additional hosts if any
      auto const & add_hosts = additional_hosts();
      hosts_.insert(add_hosts.begin(), add_hosts.end());
      
      // add my ips
      net::string_vector my_ips{net::get_own_ips(true)};
      hosts.insert(my_ips.begin(), my_ips.end());
      
      // add discovered endpoints too
      hosts_.insert(ip_discovery_client::get_ip(cfg_client.get_endpoint_client()));
      hosts_.insert("*");
    }
  }
  
  const util::zmq_socket_wrapper::host_set &
  server_base::hosts() const
  {
    return hosts_;
  }
  
  const util::zmq_socket_wrapper::host_set &
  server_base::additional_hosts() const
  {
    static util::zmq_socket_wrapper::host_set empty;
    return empty;
  }
  
  const std::string &
  server_base::name() const
  {
    return name_;
  }
  
  bool
  server_base::poll_socket(util::zmq_socket_wrapper & s,
                           unsigned long timeout_ms)
  {
    // interested in incoming messages
    zmq::pollitem_t poll_item{
      s.get(),
      0,
      ZMQ_POLLIN,
      0
    };
    
    // willing to wait for 3s for new messages
    if( zmq::poll(&poll_item, 1, timeout_ms) == -1 ||
       !(poll_item.revents & ZMQ_POLLIN) )
    {
      return false;
    }
    else
    {
      return true;
    }
  }

}}

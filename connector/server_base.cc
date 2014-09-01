#include "server_base.hh"
#include "config_client.hh"
#include "ip_discovery_client.hh"
#include <util/net.hh>

using namespace virtdb;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  server_base::server_base(config_client & cfg_client)
  {
    // collect hosts to bind to
    zmq_socket_wrapper::host_set hosts;
    {
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
  
}}

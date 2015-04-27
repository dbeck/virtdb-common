#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "server_base.hh"
#include "config_client.hh"
#include "ip_discovery_client.hh"
#include <util/net.hh>
#include <util/hex_util.hh>
#include <xxhash.h>

#ifndef NO_IPV6_SUPPORT
#define VIRTDB_SUPPORTS_IPV6 true
#else
#define VIRTDB_SUPPORTS_IPV6 false
#endif

using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  std::string
  server_base::hash_ep(const std::string & ep_string)
  {
    std::string ret;
    util::hex_util(XXH64(ep_string.c_str(), ep_string.size(), 0), ret);
    return ret;
  }
  
  server_base::server_base(server_context::sptr ctx,
                           config_client & cfg_client)
  : context_(ctx),
    name_(cfg_client.get_endpoint_client().name())
  {
    auto const & epcli = cfg_client.get_endpoint_client();
    
    // generate a hash on the endpoint service
    ep_hash_ = hash_ep(epcli.service_ep());
  }
  
  util::zmq_socket_wrapper::endpoint_set
  server_base::registered_endpoints(endpoint_client & ep_client,
                                    interface::pb::ServiceType st,
                                    interface::pb::ConnectionType ct) const
  {
    interface::pb::EndpointData ep_data;
    util::zmq_socket_wrapper::endpoint_set ret;
    
    if( ep_client.get(name(), st, ep_data) )
    {
      for( auto const & c : ep_data.connections() )
      {
        if( c.type() == ct )
        {
          ret.insert(c.address().begin(), c.address().end());
        }
      }
    }
    
    return ret;
  }
  
  interface::pb::Connection &
  server_base::conn()
  {
    return conn_;
  }
  
  const interface::pb::Connection &
  server_base::conn() const
  {
    return conn_;
  }
  
  util::zmq_socket_wrapper::host_set
  server_base::hosts(endpoint_client & ep_client) const
  {
    util::zmq_socket_wrapper::host_set ret;

    // add additional hosts if any
    auto const & add_hosts = additional_hosts();
    ret.insert(add_hosts.begin(), add_hosts.end());
    
    // add my ips
    net::string_vector my_ips{net::get_own_ips(VIRTDB_SUPPORTS_IPV6)};
    ret.insert(my_ips.begin(), my_ips.end());
    
    // add discovered endpoints too
    ret.insert(ip_discovery_client::get_ip(ep_client,
                                           ip_discovery_timeout_ms()));

    return ret;
  }
  
  const std::string &
  server_base::ep_hash() const
  {
    return ep_hash_;    
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
}}

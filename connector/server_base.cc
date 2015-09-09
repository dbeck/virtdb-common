#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <connector/server_base.hh>
#include <connector/config_client.hh>
#include <connector/ip_discovery_client.hh>
#include <util/net.hh>
#include <string>

#ifndef NO_IPV6_SUPPORT
#define VIRTDB_SUPPORTS_IPV6 true
#else
#define VIRTDB_SUPPORTS_IPV6 false
#endif

using namespace virtdb::util;

namespace virtdb { namespace connector {
    
  server_base::server_base(server_context::sptr ctx)
  : context_{ctx},
    service_name_cb_{[](server_context::sptr c){ return c->service_name(); }}
  {
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
    const server_context::hosts_set & bind_also = context_->bind_also_to();
    ret.insert(bind_also.begin(), bind_also.end());
    
    // add my ips
    net::string_vector my_ips{net::get_own_ips(VIRTDB_SUPPORTS_IPV6)};
    ret.insert(my_ips.begin(), my_ips.end());
    
    // add discovered endpoints too
    ret.insert(ip_discovery_client::get_ip(ep_client,
                                           context_->ip_discovery_timeout_ms()));
    
    if( ep_client.service_ep().find("tcp://127.0.0.1") != std::string::npos )
    {
      ret.insert("127.0.0.1");
    }

    return ret;
  }
  
  std::string
  server_base::ep_hash() const
  {
    std::string ret{context_->endpoint_hash()};
    return ret;
  }
    
  std::string
  server_base::name() const
  {
    std::string ret{context_->service_name()};
    return ret;
  }
  
  void
  server_base::override_service_name(service_name_cb cb)
  {
    service_name_cb_ = cb;
  }
  
  std::string
  server_base::service_name() const
  {
    std::string ret{"UNKNOWN"};
    if( service_name_cb_ )
      ret = service_name_cb_(context_);
    return ret;
  }
  
}}

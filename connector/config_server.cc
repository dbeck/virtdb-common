#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "config_server.hh"
#include <logger.hh>
#include <util/net.hh>
#include <util/flex_alloc.hh>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  namespace
  {
    zmq_socket_wrapper::host_set
    endpoint_hosts(const endpoint_server & ep_server)
    {
      zmq_socket_wrapper::host_set hosts;
      auto ep_global = parse_zmq_tcp_endpoint(ep_server.global_ep());
      auto ep_local = parse_zmq_tcp_endpoint(ep_server.local_ep());
      hosts.insert(ep_global.first);
      hosts.insert(ep_local.first);
      return hosts;
    }
  }

  config_server::config_server(config_client & cfg_client,
                               endpoint_server & ep_server)
  :
    rep_base_type(cfg_client,
                  std::bind(&config_server::process_replies,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&config_server::publish_config,
                            this,
                            std::placeholders::_1)),
    pub_base_type(cfg_client),
    additional_hosts_(endpoint_hosts(ep_server))
  {
    // setting up our own endpoints
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ep_server.name());
      ep_data.set_svctype(pb::ServiceType::CONFIG);
      
      // REP socket
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      
      // PUB socket
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());
      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  void
  config_server::publish_config(rep_base_type::rep_item_sptr rep)
  {
    if( rep && rep->has_name() )
    {
      std::string subscription{rep->name()};
      LOG_TRACE("publishing" << V_(subscription) << M_(*rep));
      publish(subscription,std::move(rep));
    }
  }
  
  void
  config_server::process_replies(const rep_base_type::req_item & request,
                                 rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr ret;
    LOG_TRACE("config request arrived" << M_(request));
    
    lock l(mtx_);
    auto cfg_it = configs_.find(request.name());
    
    if( cfg_it != configs_.end() && !request.configdata_size() )
    {
      ret = allocate_pub_item(cfg_it->second);
    }
    
    if( request.configdata_size() )
    {
      // save config data
      if( cfg_it != configs_.end() )
        configs_.erase(cfg_it);
      
      configs_[request.name()] = request;
    }
    
    if( !ret )
    {
      // send back the original request
      ret = allocate_pub_item(request);
    }
    
    handler(ret,false);
  }
    
  const util::zmq_socket_wrapper::host_set &
  config_server::additional_hosts() const
  {
    return additional_hosts_;
  }
  
  config_server::~config_server() { }
}}

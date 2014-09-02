#include "config_server.hh"
#include <logger.hh>
#include <util/net.hh>
#include <util/flex_alloc.hh>

using namespace virtdb;
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
    additional_hosts_(endpoint_hosts(ep_server)),
    zmqctx_(1),
    rep_base_type(cfg_client,
                  std::bind(&config_server::generate_reply,
                            this,
                            std::placeholders::_1),
                  std::bind(&config_server::publish_config,
                            this,
                            std::placeholders::_1)),
    cfg_pub_socket_(zmqctx_, ZMQ_PUB)
  {
    cfg_pub_socket_.batch_tcp_bind(hosts());
    
    // setting up our own endpoints
    {
      pb::EndpointData ep_data;
      {
        ep_data.set_name(ep_server.name());
        ep_data.set_svctype(pb::ServiceType::CONFIG);
        
        {
          // REP socket
          auto conn = ep_data.add_connections();
          conn->MergeFrom(rep_base_type::bound_to());
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
  }
  
  void
  config_server::publish_config(rep_base_type::rep_item_sptr rep)
  {
    try
    {
      int reply_size = rep->ByteSize();
      util::flex_alloc<unsigned char, 1024> reply_buffer(reply_size);
      
      if( rep->SerializeToArray(reply_buffer.get(), reply_size) )
      {
        cfg_pub_socket_.get().send(rep->name().c_str(),
                                   rep->name().length(), ZMQ_SNDMORE);
        cfg_pub_socket_.get().send(reply_buffer.get(), reply_size);
      }
    }
    catch (const std::exception & e)
    {
      std::string exception_text{e.what()};
      LOG_ERROR("couldn't serialize message" << exception_text);
    }
    catch( ... )
    {
      LOG_ERROR("unknown exception");
    }
  }
  
  config_server::rep_base_type::rep_item_sptr
  config_server::generate_reply(const rep_base_type::req_item & request)
  {
    rep_base_type::rep_item_sptr ret;
    
    lock l(mtx_);
    auto cfg_it = configs_.find(request.name());
    
    if( cfg_it != configs_.end() && !request.has_configdata() )
    {
      ret.reset(new rep_item{cfg_it->second});
    }
    
    if( request.has_configdata() )
    {
      // save config data
      if( cfg_it != configs_.end() )
        configs_.erase(cfg_it);
      
      configs_[request.name()] = request;
    }
    
    if( !ret )
    {
      ret.reset(new rep_item{request});
    }
    
    return ret;
  }
    
  const util::zmq_socket_wrapper::host_set &
  config_server::additional_hosts() const
  {
    return additional_hosts_;
  }
  
  config_server::~config_server() { }
}}

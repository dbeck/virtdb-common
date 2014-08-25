#include "config_server.hh"
#include <logger.hh>
#include <util/net.hh>
#include <util/flex_alloc.hh>

using namespace virtdb;
using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {

  config_server::config_server(config_client & cfg_client,
                               endpoint_server & ep_server)
  : zmqctx_(1),
    cfg_rep_socket_(zmqctx_, ZMQ_PULL),
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
    
    cfg_rep_socket_.batch_tcp_bind(hosts);
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
          conn->set_type(pb::ConnectionType::REQ_REP);
          
          for( auto const & ep : cfg_rep_socket_.endpoints() )
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
    zmq::pollitem_t poll_item{ cfg_rep_socket_.get(), 0, ZMQ_POLLIN, 0 };
    if( zmq::poll(&poll_item, 1, 3000) == -1 ||
       !(poll_item.revents & ZMQ_POLLIN) )
    {
      return true;
    }

    zmq::message_t message;
    if( !cfg_rep_socket_.get().recv(&message) )
      return true;

    // TODO : check shall we not send back something here???
    pb::Config request;
    if( !message.data() || !message.size())
      return true;
    
    bool message_parsed = false;
    try
    {
      if( request.ParseFromArray(message.data(), message.size()) )
      {
        std::cerr << "config request arrived: \n" << request.DebugString() << "\n";
        message_parsed = true;
      }
    }
    catch (const std::exception & e)
    {
      // ParseFromArray may throw exceptions here but we don't care
      // of it does
      std::string exception_text{e.what()};
      LOG_ERROR("couldn't parse message" << exception_text);
    }
    catch( ... )
    {
      LOG_ERROR("unknown exception");
    }
    
    // if any issue happened we send back the original message
    if( !message_parsed )
    {
      LOG_ERROR("sendin back original config data because the message couldn't be parsed");
      cfg_rep_socket_.get().send( message.data(), message.size() );
      return true;
    }

    {
      lock l(mtx_);
      auto cfg_it = configs_.find(request.name());
      bool reply_sent = false;
      
      try
      {
        if( cfg_it != configs_.end() && !request.has_configdata() )
        {
          // we have some data to send back, so let's serialize it
          int reply_size = cfg_it->second.ByteSize();
          util::flex_alloc<unsigned char, 512> reply_buffer(reply_size);
          
          if( cfg_it->second.SerializeToArray(reply_buffer.get(), reply_size) )
          {
            cfg_rep_socket_.get().send(reply_buffer.get(), reply_size);
            reply_sent = true;
          }
        }
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch (...)
      {
        LOG_ERROR("unknown excpetion");
      }
      
      if( !reply_sent )
      {
        cfg_rep_socket_.get().send(message.data(), message.size());
      }

      if( request.has_configdata() )
      {
        // save config data
        if( cfg_it != configs_.end() )
          configs_.erase(cfg_it);
        
        configs_[request.name()] = request;

        // publish config data
        cfg_pub_socket_.get().send(request.name().c_str(), request.name().length(), ZMQ_SNDMORE);
        cfg_pub_socket_.get().send(message.data(), message.size());
      }
    }
    return true;
  }
}}

#include "endpoint_server.hh"
#include <util/net.hh>
#include <util/flex_alloc.hh>
#include <logger.hh>
#include <sstream>
#include <iostream>

using namespace virtdb;
using namespace virtdb::util;
using namespace virtdb::interface;

namespace virtdb { namespace connector {
  
  endpoint_server::endpoint_server(const std::string & svc_endpoint,
                                   const std::string & service_name)
  : name_(service_name),
    local_ep_(svc_endpoint),
    zmqctx_(1),
    ep_rep_socket_(zmqctx_, ZMQ_REP),
    ep_pub_socket_(zmqctx_, ZMQ_PUB),
    worker_(std::bind(&endpoint_server::worker_function,this))
  {
    ep_rep_socket_.bind( svc_endpoint.c_str() );
    // pub sub sockets, kernel to chose a port available on all IPs
    ep_pub_socket_.bind( "tcp://*:*" );
    
    int pub_zmq_port = 0;
    {
      // TODO: refactor to separate class ...
      char last_zmq_endpoint[512];
      last_zmq_endpoint[0] = 0;
      size_t opt_size = sizeof(last_zmq_endpoint);
      ep_pub_socket_.getsockopt(ZMQ_LAST_ENDPOINT, last_zmq_endpoint, &opt_size);
      last_zmq_endpoint[sizeof(last_zmq_endpoint)-1] = 0;
      if( opt_size > 0 )
      {
        char * ptr = last_zmq_endpoint+opt_size;
        while( ptr > last_zmq_endpoint )
        {
          if( *ptr == ':' )
          {
            pub_zmq_port = atoi(ptr+1);
            break;
          }
          --ptr;
        }
      }
    }
    
    {
      // add discovery endpoints
      pb::EndpointData   discovery_endpoint;
      
      size_t discovery_address_count = 0;
      discovery_endpoint.set_name("ip_discovery");
      discovery_endpoint.set_svctype(pb::ServiceType::IP_DISCOVERY);
      {
        auto disc_ep = discovery_.endpoints();
        auto conn = discovery_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::RAW_UDP);
        
        for( auto const & it : disc_ep )
        {
          auto address = conn->add_address();
          *address = it;
          ++discovery_address_count;
        }
      }
      if( discovery_address_count > 0 )
      {
        endpoints_.insert(discovery_endpoint);
      }
    }
    
    {
      // add self endpoints
      pb::EndpointData   self_endpoint;
      
      size_t svc_config_address_count = 0;
      self_endpoint.set_name(service_name);
      self_endpoint.set_svctype(pb::ServiceType::ENDPOINT);
      {
        auto conn = self_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::REQ_REP);
        // add address parameter
        auto address = conn->add_address();
        *address = svc_endpoint;
        ++svc_config_address_count;
      }
      
      // pub sub sockets, one each on each IPs on a kernel chosen port
      if( pub_zmq_port )
      {
        // TODO : refactor self IP discovery to a support class...
        auto conn = self_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::PUB_SUB);
        
        auto ips = util::net::get_own_ips(true);
        for( auto const & ip : ips )
        {
          std::ostringstream os;
          os << "tcp://";
          if( ip.find(':') == std::string::npos )
            os << ip << ":" << pub_zmq_port; // ipv4
          else
            os << '[' << ip << "]:" << pub_zmq_port; //ipv6
          
          auto address = conn->add_address();
          *address = os.str();
          ++svc_config_address_count;
        }
      }
      if( svc_config_address_count > 0 )
        endpoints_.insert(self_endpoint);
    }
    
    worker_.start();
  }
  
  bool
  endpoint_server::worker_function()
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    zmq::pollitem_t poll_item{ ep_rep_socket_, 0, ZMQ_POLLIN, 0 };
    if( zmq::poll(&poll_item, 1, 3000) == -1 ||
        !(poll_item.revents & ZMQ_POLLIN) )
    {
      return true;
    }
    
    zmq::message_t message;
    if( !ep_rep_socket_.recv(&message) )
      return true;
    
    pb::Endpoint request;
    if( !message.data() || !message.size())
      return true;
    
    try
    {
      if( request.ParseFromArray(message.data(), message.size()) )
      {
        for( int i=0; i<request.endpoints_size(); ++i )
        {
          // ignore endpoints with no connections
          if( request.endpoints(i).connections_size() > 0 &&
              request.endpoints(i).svctype() != pb::ServiceType::NONE )
          {
            // remove old endpoints if exists
            auto it = endpoints_.find(request.endpoints(i));
            if( it != endpoints_.end() )
              endpoints_.erase(it);
            
            // insert endpoint
            endpoints_.insert(request.endpoints(i));
          }
        }
        std::cerr << "endpoint request arrived: \n" << request.DebugString() << "\n";
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
    
    pb::Endpoint reply_data;
    
    // filling the reply
    for( auto const & ep_data : endpoints_ )
    {
      auto ep_ptr = reply_data.add_endpoints();
      ep_ptr->MergeFrom(ep_data);
    }

    int reply_size = reply_data.ByteSize();
    if( reply_size > 0 )
    {
      util::flex_alloc<unsigned char, 4096> reply_msg(reply_size);
      bool serialzied = reply_data.SerializeToArray(reply_msg.get(),reply_size);
      if( serialzied )
      {
        // send reply
        ep_rep_socket_.send(reply_msg.get(), reply_size);
          
        // publish new messages one by one, so subscribers can choose what to
        // receive
        for( int i=0; i<request.endpoints_size(); ++i )
        {
          auto ep = request.endpoints(i);
          if( ep.svctype() != pb::ServiceType::NONE &&
              ep.connections_size() > 0 )
          {
            pb::Endpoint publish_ep;
            publish_ep.add_endpoints()->MergeFrom(request.endpoints(i));
            
            int pub_size = publish_ep.ByteSize();
            util::flex_alloc<unsigned char, 512> pub_buffer(pub_size);
            if( publish_ep.SerializeToArray(pub_buffer.get(), pub_size) )
            {
              // generate channel key for subscribers
              std::ostringstream os;
              os << ep.svctype() << '.' << ep.name();
              std::string subscription{os.str()};
              ep_pub_socket_.send(subscription.c_str(), subscription.length(), ZMQ_SNDMORE);
              ep_pub_socket_.send(pub_buffer.get(), pub_size);
              //std::cerr << "Published (" << subscription << ")\n" << publish_ep.DebugString() << "\n";
            }
          }
        }
      }
      else
      {
        LOG_ERROR( "couldn't serialize Endpoint reply message." << V_(reply_size) );
      }
    }

    return true;
  }

  endpoint_server::~endpoint_server()
  {
    worker_.stop();
  }

  const std::string &
  endpoint_server::local() const
  {
    return local_ep_;
  }
  
  const std::string &
  endpoint_server::name() const
  {
    return name_;
  }
}}

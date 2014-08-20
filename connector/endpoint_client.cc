#include "endpoint_client.hh"
#include <svc_config.pb.h>
#include <util.hh>
#include <logger.hh>

using namespace virtdb;
using namespace virtdb::interface;
using namespace virtdb::util;
using namespace virtdb::logger;

namespace virtdb { namespace connector {
  
  const std::string & endpoint_client::name() const { return name_; }
  
  endpoint_client::endpoint_client(const std::string & svc_config_ep,
                                   const std::string & service_name)
  : name_(service_name),
    zmqctx_(1),
    ep_req_socket_(zmqctx_, ZMQ_REQ),
    ep_sub_socket_(zmqctx_,ZMQ_SUB),
    barrier_(2),
    stop_(false),
    worker_(std::bind(&endpoint_client::worker_entry,this))
  {
    process_info::set_app_name(name_);
    ep_req_socket_.connect(svc_config_ep.c_str());
    
    // setup a monitor so handle_endpoint_data will connect
    // the sub socket when we receive one
    watch(pb::ServiceType::ENDPOINT,
          [this](const pb::EndpointData & data){
            for( int i=0; i<data.connections_size(); ++i )
            {
              auto conn = data.connections(i);
              if( conn.type() == pb::ConnectionType::PUB_SUB )
              {
                for( int ii=0; ii<conn.address_size(); ++ii )
                {
                  try
                  {
                    // TODO : revise this later : only one subscription is allowed
                    if( this->ep_sub_socket_.connected() )
                      this->ep_sub_socket_.close();

                    this->ep_sub_socket_.connect(conn.address(ii).c_str());
                    // telling that we are done and don't want more addresses
                    return false;
                  }
                  catch (...)
                  {
                    // ignore connection failure
                  }
                }
              }
            }
            return true;
          });
    
    pb::Endpoint diag_ep;
    auto ep_data = diag_ep.add_endpoints();
    ep_data->set_name(svc_config_ep);
    ep_data->set_svctype(pb::ServiceType::NONE);
    int ep_size = diag_ep.ByteSize();
    
    if( ep_size > 0 )
    {
      flex_alloc<char, 256> buffer(ep_size);
      bool serialized = diag_ep.SerializeToArray(buffer.get(), ep_size);
      if( !serialized )
      {
        THROW_("Couldn't serialize our own endpoint data");
      }
      ep_req_socket_.send( buffer.get(), ep_size );
      zmq::message_t msg;
      ep_req_socket_.recv(&msg);
      
      if( !msg.data() || !msg.size() )
      {
        THROW_("invalid Endpoint message received");
      }
      
      pb::Endpoint peers;
      serialized = peers.ParseFromArray(msg.data(), msg.size());
      if( !serialized )
      {
        THROW_("couldn't process peer Endpoints");
      }
      
      for( int i=0; i<peers.endpoints_size(); ++i )
        handle_endpoint_data(peers.endpoints(i));
    }
    
    barrier_.wait();
  }
  
  void
  endpoint_client::worker_entry()
  {
    barrier_.wait();
    while( stop_ == false )
    {
      if( !ep_sub_socket_.connected() )
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else
      {
        zmq::message_t msg;
        if( ep_sub_socket_.recv(&msg) )
        {
          if( !msg.data() || !msg.size() )
          {
            // TODO : publisher ???? name, address, ???
            LOG_ERROR("empty endpoint message arrived from the publisher");
            continue;
          }
          
          pb::Endpoint peers;
          bool serialized = peers.ParseFromArray(msg.data(), msg.size());
          if( !serialized )
          {
            LOG_ERROR("couldn't process peer Endpoints from publisher");
            continue;
          }
          
          for( int i=0; i<peers.endpoints_size(); ++i )
            handle_endpoint_data(peers.endpoints(i));
        }
      }
    }
  }
  
  void
  endpoint_client::handle_endpoint_data(const interface::pb::EndpointData & ep)
  {
    std::lock_guard<std::mutex> lock(mtx_);

    {
      // run monitors first
      auto it = monitors_.find( ep.svctype() );
      if( it != monitors_.end() )
      {
        for( auto & m : it->second )
        {
          fire_monitor(m, ep);
        }
      }
    }

    {
      // remove if exists
      auto it = endpoints_.find(ep);
      if( it != endpoints_.end() )
        endpoints_.erase(it);
      
      // insert the new one
      endpoints_.insert(ep);
    }
    
    std::cerr << "handled: \n" << ep.DebugString() << "\n\n";
  }
  
  void
  endpoint_client::watch(interface::pb::ServiceType st,
                         monitor m)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    {
      auto it = monitors_.find(st);
      if( it == monitors_.end() )
      {
        auto rit = monitors_.insert(std::make_pair(st,monitor_vector()));
        it = rit.first;
      }
      it->second.push_back(m);
    }
    // iterate over the current endpoint set, so the new monitor will
    // see the old entries
    {
      for( const auto & ep : endpoints_ )
      {
        if( ep.svctype() == st )
        {
          if( !fire_monitor(m, ep) ) break;
        }
      }
    }
  }

  bool
  endpoint_client::fire_monitor(monitor & m,
                                const interface::pb::EndpointData & ep)
  {
    bool ret = true;
    
    try
    {
      ret = m(ep);
    }
    catch( const std::exception & e )
    {
      std::string text{e.what()};
      LOG_ERROR("caught exception" << V_(text));
    }
    catch( ... )
    {
      LOG_ERROR("caught unknown exception");
    }
    
    return ret;
  }
  
  endpoint_client::~endpoint_client()
  {
    if( worker_.joinable() )
      worker_.join();
  }
  
}}

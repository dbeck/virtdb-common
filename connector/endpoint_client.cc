#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "endpoint_client.hh"
#include <svc_config.pb.h>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include <util/constants.hh>
#include <logger.hh>

using namespace virtdb::interface;
using namespace virtdb::util;
using namespace virtdb::logger;

namespace virtdb { namespace connector {
  
  const std::string & endpoint_client::name()        const { return name_; }
  const std::string & endpoint_client::service_ep()  const { return service_ep_; }
  
  void
  endpoint_client::wait_valid_sub()
  {
    ep_sub_socket_.wait_valid();
  }
  
  void
  endpoint_client::wait_valid_req()
  {
    ep_req_socket_.wait_valid();
  }
  
  bool
  endpoint_client::wait_valid_sub(uint64_t timeout_ms)
  {
    return ep_sub_socket_.wait_valid(timeout_ms);
  }
  
  bool
  endpoint_client::wait_valid_req(uint64_t timeout_ms)
  {
    return ep_req_socket_.wait_valid(timeout_ms);
  }
  
  void
  endpoint_client::reconnect()
  {
    ep_sub_socket_.reconnect_all();
    ep_req_socket_.reconnect_all();
  }
  
  endpoint_client::endpoint_client(client_context::sptr ctx,
                                   const std::string & svc_config_ep,
                                   const std::string & service_name)
  : context_{ctx},
    service_ep_{svc_config_ep},
    name_{service_name},
    zmqctx_{1},
    ep_req_socket_{zmqctx_, ZMQ_REQ},
    ep_sub_socket_{zmqctx_,ZMQ_SUB},
    worker_(std::bind(&endpoint_client::worker_function,this),
            /* the preferred way is to rethrow exceptions if any on the other
               thread, rather then die */
            10, false),
    queue_(1,std::bind(&endpoint_client::async_handle_data,
                       this,
                       std::placeholders::_1))
  {
    if( svc_config_ep.empty() ) { THROW_("config service endpoint is empty"); }
    if( service_name.empty() )  { THROW_("service name is empty"); }

    process_info::set_app_name(name_);

    try
    {
      ep_req_socket_.connect(svc_config_ep.c_str());
    }
    catch(const std::exception & e)
    {
      LOG_ERROR("exception during connect to" << V_(svc_config_ep) << E_(e));
      std::ostringstream os;
      os << "exception during connect to: " <<  svc_config_ep << " exc: " << e.what();
      LOG_ERROR(V_(os.str()));
      THROW_(os.str());
    }
    
    // setup a monitor so handle_endpoint_data will connect
    // the sub socket when we receive one
    watch(pb::ServiceType::ENDPOINT,
          [this](const pb::EndpointData & ep) {
            
            for( int i=0; i<ep.connections_size(); ++i )
            {
              auto conn = ep.connections(i);
              if( conn.type() == pb::ConnectionType::PUB_SUB )
              {
                if( !ep_sub_socket_.connected_to(conn.address().begin(),
                                                 conn.address().end()) )
                {
                  for( int ii=0; ii<conn.address_size(); ++ii )
                  {
                    try
                    {
                      // TODO : revise this later : only one subscription is allowed
                      ep_sub_socket_.connect(conn.address(ii).c_str());
                      ep_sub_socket_.get().setsockopt(ZMQ_SUBSCRIBE, "*", 0);
                      return;
                    }
                    catch (const std::exception & e)
                    {
                      std::string text{e.what()};
                      LOG_ERROR("exception caught" << V_(text));
                    }
                    catch (...)
                    {
                      LOG_ERROR("unknown exception caught");
                      // ignore connection failure
                    }
                  }
                }
              }
            }
          });

    pb::EndpointData ep_data;
    ep_data.set_name(service_name);
    ep_data.set_svctype(pb::ServiceType::NONE);
    ep_data.set_cmd(pb::EndpointData::LIST);
    ep_data.set_validforms(100);
    register_endpoint(ep_data);
    worker_.start();
  }
  
  void
  endpoint_client::register_endpoint(const interface::pb::EndpointData & ep_data,
                                     monitor m)
  {
    if( !m ) { THROW_("invalid monitor in endpoint client"); }
    if( ep_data.ByteSize() <= 0 )   { THROW_("empty EndpointData"); }
    if( !ep_data.IsInitialized() )  { THROW_("ep_data is not initialized"); }
    if( !ep_data.has_name() )       { THROW_("missing name from ep_data"); }
    if( this->queue_.stopped() )    { THROW_("invalid endpoint client state"); }
    
    pb::Endpoint ep;
    auto ep_data_ptr = ep.add_endpoints();
    
    if( !ep_data_ptr ) { THROW_("cannot add new ep_data to endpoint set"); }

    try
    {
      ep_data_ptr->MergeFrom(ep_data);
    }
    catch (const std::exception & e)
    {
      LOG_ERROR("Failed to merge" << M_(ep_data) << E_(e));
      THROW_("couldn't merge EdpointData");
    }

    int ep_size = ep.ByteSize();
    
    if( ep_size > 0 )
    {
      flex_alloc<char, 256> buffer(ep_size);
      bool serialized = false;
      
      try
      {
        serialized = ep.SerializeToArray(buffer.get(), ep_size);
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("Failed to serialize" << M_(ep) << E_(e));
        serialized = false;
      }
      
      if( !serialized ) { THROW_("Couldn't serialize endpoint data"); }
      
      zmq::message_t tmp_msg(0);
      bool message_sent = false;
      try
      {
        message_sent = ep_req_socket_.send( buffer.get(), ep_size );
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("Failed to send" << M_(ep) << E_(e));
        message_sent = false;
      }
      
      if( !message_sent ) { THROW_("Couldn't send enpoint message"); }
      
      {
        int i=0;
        bool ok = false;
        for( ; i<20; ++i )
        {
          if( !ep_req_socket_.poll_in(DEFAULT_TIMEOUT_MS,
                                      SHORT_TIMEOUT_MS) )
          {
            LOG_ERROR("no response for EndpointMessage" <<  M_(ep_data) << V_(i));
          }
          else
          {
            ok = true;
            break;
          }
        }
        if( !ok )
        {
          THROW_("no response for EndpointMessage. giving up");
        }
      }
      
      if( !ep_req_socket_.get().recv(&tmp_msg) )
      {
        THROW_("failed to receive Endpoint message");
      }
      
      if( !tmp_msg.data() || !tmp_msg.size() )
      {
        THROW_("invalid Endpoint message received");
      }
      
      // copy out data from the ZMQ message
      auto msg_size = tmp_msg.size();
      util::flex_alloc<unsigned char, 64> msg(msg_size);
      ::memcpy(msg.get(), tmp_msg.data(), msg_size);
      
      pb::Endpoint peers;
      serialized = peers.ParseFromArray(msg.get(), msg_size);
      if( !serialized )
      {
        THROW_("couldn't process peer Endpoints");
      }
      
      for( int i=0; i<peers.endpoints_size(); ++i )
      {
        try
        {
          m(peers.endpoints(i));
        }
        catch( const std::exception & e)
        {
          LOG_ERROR("exception caught" << E_(e) << "when processing" << M_(ep_data));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception caught" << "when processing" << M_(ep_data));
        }
        queue_.push(peers.endpoints(i));
      }
    }
    else
    {
      THROW_("empty endpoint data");
    }
  }
  
  void
  endpoint_client::async_handle_data(ep_data_item ep)
  {
    try
    {
      handle_endpoint_data(ep);
    }
    catch( const std::exception & e )
    {
      LOG_ERROR("caught exception" << E_(e) << "while processing" << M_(ep));
    }
    catch( ... )
    {
      LOG_ERROR("caught unknown exception while processing" << M_(ep));
    }
  }
  
  bool
  endpoint_client::worker_function()
  {
    try
    {
      if( !ep_sub_socket_.wait_valid(util::DEFAULT_TIMEOUT_MS) )
        return true;
      
      if( !ep_sub_socket_.poll_in(util::DEFAULT_TIMEOUT_MS,
                                  util::TINY_TIMEOUT_MS) )
        return true;
    }
    catch (const zmq::error_t & e)
    {
      LOG_ERROR("zmq::poll failed with exception" << E_(e) << "delaying subscription loop");
      return true;
    }
    
    try
    {
      zmq::message_t msg(0);

      if( ep_sub_socket_.get().recv(&msg) )
      {
        if( !msg.data() || !msg.size() )
        {
          // TODO : publisher ???? name, address, ???
          LOG_ERROR("empty endpoint message arrived from the publisher");
          return true;
        }
        
        // assume this is a proper pub message
        const char * msg_ptr = (const char *)msg.data();
        if( msg.more() && msg_ptr[0] >= '0' && msg_ptr[0] <= '9' )
        {
          ep_sub_socket_.get().recv(&msg);
        }
        
        pb::Endpoint peers;
        bool serialized = peers.ParseFromArray(msg.data(), msg.size());
        if( !serialized )
        {
          LOG_ERROR("couldn't process peer Endpoints from publisher");
          return true;
        }
        
        for( int i=0; i<peers.endpoints_size(); ++i )
        {
          queue_.push(peers.endpoints(i));
        }
      }
      else
      {
        LOG_ERROR("ep_sub_socket_.recv() failed");
      }
    }
    catch (const zmq::error_t & e)
    {
      LOG_ERROR("zmq::poll failed with exception" << E_(e) << "delaying subscription loop");
      return true;
    }
    catch (const std::exception & e)
    {
      LOG_ERROR("couldn't parse message" << E_(e));
    }
    catch( ... )
    {
      LOG_ERROR("unknown exception");
    }
    return true;
  }
  
  void
  endpoint_client::handle_endpoint_data(const interface::pb::EndpointData & ep)
  {
    if( ep.svctype() == interface::pb::ServiceType::NONE )
    {
      return;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    bool add_message = (ep.has_cmd() == false || ep.cmd() == pb::EndpointData::ADD );
    bool new_data    = false;
    
    if( ep.connections_size() > 0 && add_message )
    {
      auto it = endpoints_.find(ep);
      if( it != endpoints_.end() )
      {
        // check sizes first
        if( it->ByteSize() != ep.ByteSize() )
        {
          new_data = true;
        }
        else
        {
          // check data content tooo
          util::flex_alloc<unsigned char, 1024> old_ep(it->ByteSize());
          util::flex_alloc<unsigned char, 1024> new_ep(ep.ByteSize());
          
          if( it->SerializeToArray(old_ep.get(), it->ByteSize()) &&
              ep.SerializeToArray(new_ep.get(), ep.ByteSize()) )
          {
            // content is different
            if( ::memcmp(old_ep.get(), new_ep.get(), it->ByteSize()) != 0 )
              new_data = true;
          }
        }
        
        // remove old data if new is different
        if( new_data )
        {
          endpoints_.erase(it);
        }
      }
      else
      {
        new_data = true;
      }
    }
    
    if( new_data )
    {
      LOG_TRACE("handle new endpoint data" << M_(ep) << V_((int)ep.svctype()) );

      // run monitors first
      auto it = monitors_.find( ep.svctype() );
      if( it != monitors_.end() &&
          ep.connections_size() > 0 )
      {
        for( auto & m : it->second )
        {
          fire_monitor(m, ep);
        }
      }
      
      endpoints_.insert(ep);
    }
  }
  
  void
  endpoint_client::remove_watches(interface::pb::ServiceType st)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = monitors_.find(st);
    if( it != monitors_.end() )
      monitors_.erase(it);
  }

  void
  endpoint_client::remove_watches()
  {
    std::lock_guard<std::mutex> lock(mtx_);
    monitors_.clear();
  }
  
  void
  endpoint_client::watch(interface::pb::ServiceType st,
                         monitor m)
  {
    if( !m ) { THROW_("invalid monitor in endpoint client"); }
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
          fire_monitor(m, ep);
        }
      }
    }
  }
  
  bool
  endpoint_client::get(const std::string & ep_name,
                       interface::pb::ServiceType st,
                       interface::pb::EndpointData & result) const
  {
    interface::pb::EndpointData dt;
    dt.set_name(ep_name);
    dt.set_svctype(st);
    {
      std::lock_guard<std::mutex> lock(mtx_);
      auto it = endpoints_.find(dt);
      if( it != endpoints_.end() )
      {
        result = *it;
        return true;
      }
    }
    return false;
  }
  
  bool
  endpoint_client::get(interface::pb::ServiceType st,
                       interface::pb::EndpointData & result) const
  {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      for( auto const & e : endpoints_ )
      {
        if( e.svctype() == st )
        {
          result = e;
          return true;
        }
      }
    }
    return false;
  }

  void
  endpoint_client::fire_monitor(monitor & m,
                                const interface::pb::EndpointData & ep)
  {
    if( ep.svctype() == interface::pb::ServiceType::NONE )
    {
      // ignore this service type and save others time too
      // by returning here
    }
    
    try
    {
      m(ep);
    }
    catch( const std::exception & e )
    {
      LOG_ERROR("caught exception" << E_(e));
    }
    catch( ... )
    {
      LOG_ERROR("caught unknown exception");
    }
  }
  
  endpoint_client::~endpoint_client()
  {
    cleanup();
  }
  
  void
  endpoint_client::cleanup()
  {
    remove_watches();
    worker_.stop();
    queue_.stop();
  }
  
  void
  endpoint_client::rethrow_error()
  {
    worker_.rethrow_error();
  }
  
}}

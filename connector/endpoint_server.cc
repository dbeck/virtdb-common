#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "endpoint_server.hh"
#include <connector/server_base.hh>
#include <util/net.hh>
#include <util/flex_alloc.hh>
#include <util/constants.hh>
#include <util/relative_time.hh>
#include <logger.hh>
#include <sstream>
#include <iostream>
#include <fstream>

#ifndef NO_IPV6_SUPPORT
#define VIRTDB_SUPPORTS_IPV6 true
#else
#define VIRTDB_SUPPORTS_IPV6 false
#endif

using namespace virtdb::util;
using namespace virtdb::interface;
using namespace virtdb::logger;

namespace virtdb { namespace connector {
  
  util::zmq_socket_wrapper::host_set
  endpoint_server::endpoint_hosts() const
  {
    using namespace virtdb::util;
    zmq_socket_wrapper::host_set hosts;
    auto ep_global = parse_zmq_tcp_endpoint(global_ep());
    auto ep_local = parse_zmq_tcp_endpoint(local_ep());
    hosts.insert(ep_global.first);
    hosts.insert(ep_local.first);
    return hosts;
  }
  
  const std::string & endpoint_server::local_ep() const { return local_ep_; }
  const std::string & endpoint_server::global_ep() const { return global_ep_; }
  
  endpoint_server::endpoint_server(server_context::sptr ctx)
  : context_{ctx},
    local_ep_{ctx->endpoint_svc_addr()},
    global_ep_{ctx->endpoint_svc_addr()},
    zmqctx_(1),
    ep_router_socket_(zmqctx_, ZMQ_ROUTER),
    ep_pub_socket_(zmqctx_, ZMQ_PUB),
    worker_{std::bind(&endpoint_server::worker_function,this),
            /* the preferred way is to rethrow exceptions if any on the other
               thread, rather then die */
            10,false},
    on_up_down_{[](const std::string & name, bool is_up){}}
  {
    process_info::set_app_name(ctx->service_name());
    
    // collect hosts to bind to
    zmq_socket_wrapper::host_set hosts;
    {
      auto ep_global = parse_zmq_tcp_endpoint(global_ep());
      auto ep_local = parse_zmq_tcp_endpoint(local_ep());

      // add my ips
      net::string_vector my_ips{net::get_own_ips(VIRTDB_SUPPORTS_IPV6)};
      hosts.insert(my_ips.begin(), my_ips.end());
      
      // add endpoint_server hostnames too
      hosts.insert(ep_global.first);
      hosts.insert(ep_local.first);
      hosts.insert("*");
    }
    ep_router_socket_.bind(ctx->endpoint_svc_addr().c_str());
    ep_pub_socket_.batch_tcp_bind(hosts);
    
    // start worker before we report endpoints
    worker_.start();
    
    {
      // add discovery endpoints
      pb::EndpointData   discovery_endpoint;
      
      size_t discovery_address_count = 0;
      discovery_endpoint.set_name(ctx->service_name()); // "ip_discovery"
      discovery_endpoint.set_svctype(pb::ServiceType::IP_DISCOVERY);
      {
        auto disc_ep = discovery_.endpoints();
        auto conn = discovery_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::RAW_UDP);
        
        for( auto const & it : disc_ep )
        {
          *(conn->add_address()) = it;
          ++discovery_address_count;
        }
      }
      if( discovery_address_count > 0 )
      {
        std::unique_lock<std::mutex> l(mtx_);
        endpoints_.insert(discovery_endpoint);
      }
    }
    
    {
      // add self endpoints
      pb::EndpointData   self_endpoint;
      
      size_t svc_config_address_count = 0;
      self_endpoint.set_name(ctx->service_name());
      self_endpoint.set_svctype(pb::ServiceType::ENDPOINT);
      {
        auto conn = self_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::REQ_REP);
        
        std::string svc_endpoint{ctx->endpoint_svc_addr()};
        
        if( svc_endpoint.find("0.0.0.0") == std::string::npos )
        {
          // add address parameter
          *(conn->add_address()) = svc_endpoint;
          ++svc_config_address_count;
        }
        
        for( const auto & bound_to : ep_router_socket_.endpoints() )
        {
          if( bound_to != svc_endpoint )
          {
            // filter svc_endpoint since that is already in the list
            *(conn->add_address()) = bound_to;
            ++svc_config_address_count;
          }
        }
      }
      
      // pub sub sockets, one each on each IPs on a kernel chosen port
      {
        auto conn = self_endpoint.add_connections();
        conn->set_type(pb::ConnectionType::PUB_SUB);
        
        for( const auto & bound_to : ep_pub_socket_.endpoints() )
        {
          *(conn->add_address()) = bound_to;
          ++svc_config_address_count;
        }
      }
      if( svc_config_address_count > 0 )
      {
        std::unique_lock<std::mutex> l(mtx_);
        endpoints_.insert(self_endpoint);
      }
    }
  }
  
  void
  endpoint_server::on_up_down(on_up_down_fun monitor)
  {
    std::unique_lock<std::mutex> l(mtx_);
    if( !monitor )
      monitor = [](const std::string & name, bool is_up){};
    on_up_down_ = monitor;
  }
  
  bool
  endpoint_server::add_endpoint_data(const interface::pb::EndpointData & epr)
  {
    std::unique_lock<std::mutex> l(mtx_);
    bool inserted = false;
    bool new_data = false;
    
    // ignore endpoints with no connections
    if( epr.connections_size() > 0 &&
        epr.svctype() != pb::ServiceType::NONE &&
       (epr.has_cmd() == false || epr.cmd() == pb::EndpointData::ADD ))
    {
      // remove old endpoints if exists
      auto it = endpoints_.find(epr);
      if( it != endpoints_.end() )
      {
        // check sizes first
        if( it->ByteSize() != epr.ByteSize() )
        {
          new_data = true;
        }
        else
        {
          // check data content tooo
          util::flex_alloc<unsigned char, 4096> old_ep(it->ByteSize());
          util::flex_alloc<unsigned char, 4096> new_ep(epr.ByteSize());
          
          if( it->SerializeToArray(old_ep.get(), it->ByteSize()) &&
              epr.SerializeToArray(new_ep.get(), epr.ByteSize()) )
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
        // insert endpoint
        endpoints_.insert(epr);
        new_data = true;
      }
      
      inserted = true;
      on_up_down_(epr.name(), true);
    }
    
    if( epr.has_validforms() && inserted )
    {
      std::string exp_svc_name{epr.name()};
      pb::ServiceType exp_svc_type{epr.svctype()};
      
      uint64_t now     = util::relative_time::instance().get_msec();
      uint64_t expiry  = now + epr.validforms();
      
      // set maximum vaildity for the component
      std::ostringstream os;
      os << epr.svctype() << ' ' << epr.name();
      std::string subscription{os.str()};
      keep_alive_[subscription] = expiry;
      
      // schedule removal
      timer_svc_.schedule(epr.validforms()+SHORT_TIMEOUT_MS,
                          [this,
                           exp_svc_name,
                           exp_svc_type,
                           subscription]() {
                            {
                              std::unique_lock<std::mutex> l(mtx_);
                              auto it = keep_alive_.find(subscription);
                              if( it == keep_alive_.end() )
                                return false;
                              
                              uint64_t now     = util::relative_time::instance().get_msec();
                              uint64_t expiry  = it->second;
                              if( now >= expiry )
                              {
                                pb::EndpointData to_remove;
                                to_remove.set_name(exp_svc_name);
                                to_remove.set_svctype(exp_svc_type);
                                to_remove.set_cmd(pb::EndpointData::REMOVE);
                                endpoints_.erase(to_remove);
                                LOG_INFO("Endpoint expired" << M_(to_remove));
                                publish_endpoint(to_remove);
                                keep_alive_.erase(it);
                                on_up_down_(exp_svc_name, false);
                              }
                            }
                            // non-periodic check it is
                            return false;
                          });
    }
    return new_data;
  }
  
  bool
  endpoint_server::worker_function()
  {
    if( !ep_router_socket_.poll_in(util::DEFAULT_TIMEOUT_MS,
                                   util::SHORT_TIMEOUT_MS) )
      return true;
    
    // poll said we have data ...
    zmq::message_t tmp(0);
    
    // read ID
    if( !ep_router_socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive ID"); return true; }
    if( !tmp.data() || !tmp.size()) { LOG_ERROR("ID has invalid content" << P_(tmp.data()) << V_(tmp.size()) ); return true; }
    if( !tmp.more() )               { LOG_ERROR("No more data after ID" << P_(tmp.data()) << V_(tmp.size()) ); return true; }
    
    // copy out data from the ZMQ message
    auto id_size = tmp.size();
    util::flex_alloc<unsigned char, 64> id(tmp.size());
    memcpy(id.get(), tmp.data(), id_size);
    
    // separator
    if( !ep_router_socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive separator"); return true; }
    if( !tmp.more() )               { LOG_ERROR("No more data after separator" << P_(tmp.data()) << V_(tmp.size()) ); return true; }
    
    // message
    if( !ep_router_socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive message"); return true; }
    if( !tmp.data() || !tmp.size()) { LOG_ERROR("message has invalid content" << P_(tmp.data()) << V_(tmp.size()) ); return true; }
    
    // copy out data from the ZMQ message
    auto msg_size = tmp.size();
    util::flex_alloc<unsigned char, 1024> msg(msg_size);
    memcpy(msg.get(), tmp.data(), msg_size);
    
    pb::Endpoint request;
    ep_data_set to_publish;
    
    try
    {
      if( request.ParseFromArray(msg.get(), msg_size) )
      {
        for( auto epr : request.endpoints() )
        {
          // only publish new or changed endpoints
          if( add_endpoint_data(epr) )
            to_publish.insert(epr);
        }
        LOG_TRACE("endpoint request arrived" << M_(request));
      }
    }
    catch (const std::exception & e)
    {
      // ParseFromArray may throw exceptions here but we don't care
      // of it does
      LOG_ERROR("couldn't parse message" << E_(e));
    }
    catch( ... )
    {
      LOG_ERROR("unknown exception");
    }
    
    pb::Endpoint reply_data;
    
    // filling the reply
    {
      std::unique_lock<std::mutex> l(mtx_);
      for( auto const & ep_data : endpoints_ )
      {
        auto ep_ptr = reply_data.add_endpoints();
        ep_ptr->MergeFrom(ep_data);
      }
    }

    int reply_size = reply_data.ByteSize();
    if( reply_size > 0 )
    {
      util::flex_alloc<unsigned char, 4096> reply_msg(reply_size);
      bool serialzied = reply_data.SerializeToArray(reply_msg.get(),reply_size);
      if( serialzied )
      {
        // send ID and separator first
        size_t send_ret = 0;
        if( (send_ret=ep_router_socket_.send(id.get(), id_size, ZMQ_SNDMORE)) == 0 )
        {
          LOG_ERROR("failed to send ID" <<
                    V_(send_ret) <<
                    M_(reply_data));
          return true;
        }
        
        ep_router_socket_.send("", 0, ZMQ_SNDMORE);

        // send reply
        send_ret = ep_router_socket_.send(reply_msg.get(), reply_size);
        if( !send_ret )
        {
          LOG_ERROR("failed to send reply" <<
                    V_(send_ret) <<
                    M_(reply_data));
          return true;
        }
        
        // publish new messages one by one, so subscribers can choose what to
        // receive
        for( auto epr : to_publish )
        {
          publish_endpoint(epr);
        }
      }
      else
      {
        LOG_ERROR( "couldn't serialize Endpoint reply message." << V_(reply_size) );
      }
    }
    return true;
  }
  
  void
  endpoint_server::publish_endpoint(const interface::pb::EndpointData & ep)
  {
    if( ep.svctype() != pb::ServiceType::NONE )
    {
      pb::Endpoint publish_ep;
      publish_ep.add_endpoints()->MergeFrom(ep);
      
      int pub_size = publish_ep.ByteSize();
      util::flex_alloc<unsigned char, 512> pub_buffer(pub_size);
      if( publish_ep.SerializeToArray(pub_buffer.get(), pub_size) )
      {
        // generate channel key for subscribers
        std::ostringstream os;
        os << ep.svctype() << ' ' << ep.name();
        std::string subscription{os.str()};
        
        if( !ep_pub_socket_.send(subscription.c_str(),
                                 subscription.length(),
                                 ZMQ_SNDMORE) )
        {
          LOG_ERROR("cannot send data" << V_(subscription));
          return;
        }
        if( !ep_pub_socket_.send(pub_buffer.get(),
                                 pub_size) )
        {
          LOG_ERROR("cannot send data" << M_(publish_ep));
          return;
        }
        
        LOG_TRACE("published message" << M_(publish_ep) << "to" << V_(subscription));
      }
      else
      {
        LOG_ERROR("failed to serialize message" << M_(publish_ep) << V_(ep.name()) );
      }
    }
  }
  
  void
  endpoint_server::reload_from(const std::string & path)
  {
    std::string inpath{path + "/" + server_context::hash_ep(local_ep_) + "-" + "endpoint.data"};
    std::ifstream ifs{inpath};
    if( ifs.good() )
    {
      pb::Endpoint eps;
      if( eps.ParseFromIstream(&ifs) )
      {
        for( auto const & ep: eps.endpoints() )
        {
          if( ep.name() != this->name() &&
              ep.name() != "ip_discovery" )
          {
            if( add_endpoint_data(ep) )
            {
              publish_endpoint(ep);
            }
          }
        }
      }
    }
  }
  
  void
  endpoint_server::save_to(const std::string & path)
  {
    pb::Endpoint eps;
    
    // filling the database data
    {
      std::unique_lock<std::mutex> l(mtx_);
      for( auto const & ep : endpoints_ )
      {
        auto ep_ptr = eps.add_endpoints();
        ep_ptr->MergeFrom(ep);
      }
    }
    
    std::string outpath{path + "/" + server_context::hash_ep(local_ep_) + "-" + "endpoint.data"};
    std::ofstream of{outpath};
    if( of.good() )
    {
      eps.SerializeToOstream(&of);
    }
  }

  endpoint_server::~endpoint_server()
  {
    worker_.stop();
  }
  
  void
  endpoint_server::cleanup()
  {
    worker_.stop();
    timer_svc_.cleanup();
  }
  
  void
  endpoint_server::rethrow_error()
  {
    worker_.rethrow_error();
    timer_svc_.rethrow_error();
  }

  std::string
  endpoint_server::name() const
  {
    std::string ret{context_->service_name()};
    return ret;
  }
}}

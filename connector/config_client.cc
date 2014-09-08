#include "config_client.hh"
#include <logger.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include <util/constants.hh>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  config_client::config_client(endpoint_client & ep_client,
                               const std::string & server_name)
  : req_base_type(ep_client, server_name),
    ep_client_(&ep_client),
    zmqctx_(1),
    cfg_sub_socket_(zmqctx_, ZMQ_SUB),
    worker_(std::bind(&config_client::worker_function,this))
  {
    ep_client.watch(pb::ServiceType::CONFIG,
                    [this](const pb::EndpointData & ep) {
                      return this->on_endpoint_data(ep);
                    });

    worker_.start();
  }
  
  bool
  config_client::worker_function()
  {
    try
    {
      if( !cfg_sub_socket_.wait_valid(DEFAULT_TIMEOUT_MS) )
        return true;
      
      if( !cfg_sub_socket_.poll_in(DEFAULT_TIMEOUT_MS) )
        return true;
    }
    catch (const zmq::error_t & e)
    {
      std::string text{e.what()};
      LOG_ERROR("zmq::poll failed with exception" << V_(text) << "delaying subscription loop");
      lock l(sockets_mtx_);
      cfg_sub_socket_.disconnect_all();
      return true;
    }
    
    zmq::message_t msg;
    
    if( cfg_sub_socket_.get().recv(&msg) )
    {
      if( !msg.data() || !msg.size() )
        return true;
      
      // assume this is a proper pub message
      // so we don't need to care about the first part
      const char * msg_data = static_cast<const char *>(msg.data());
      std::string subscription{msg_data,msg_data+msg.size()};
      
      bool has_monitors = false;
      {
        // TODO : handle wildcard subscription too...
        lock l(monitors_mtx_);
        auto it = monitors_.find(subscription);
        if( it != monitors_.end() )
          has_monitors = true;
      }
      
      LOG_TRACE("received message for" << V_(subscription) << V_(has_monitors));
      
      while( msg.more() )
      {
        msg.rebuild();
        cfg_sub_socket_.get().recv(&msg);
        
        if( msg.data() && msg.size() && has_monitors )
        {
          try
          {
            pb::Config cfg;
            bool parsed = cfg.ParseFromArray(msg.data(), msg.size());
            if( parsed )
            {
              lock l(monitors_mtx_);
              auto it = monitors_.find(subscription);
              if( it != monitors_.end() )
              {
                for( auto & m : it->second )
                {
                  try
                  {
                    // call monitor
                    if( !m(cfg) )
                      break;
                  }
                  catch (...)
                  {
                    // don't care about monitor exceptions to avoid
                    // log message loops. should any exception happen in a
                    // given monitor we still want other monitors to continue
                    // execution
                  }
                }
              }
            }
          }
          catch (...)
          {
            // don't care about errors here so we catch everything
            // the most likely excpetion is std::exception in ParseFromArray
            // I prefer to stay silent so malicious clients won't flood
            // our logs
          }
        }
      }
    }
    
    return true;
  }
  
  void
  config_client::watch(const std::string & name,
                       cfg_monitor mon)
  {
    bool needs_subscription = false;
    {
      lock l(monitors_mtx_);
      auto it = monitors_.find(name);
      if( it == monitors_.end() )
      {
        auto rit = monitors_.insert(std::make_pair(name,monitor_vector()));
        it = rit.first;
        needs_subscription = true;
      }
      
      it->second.push_back(mon);
    }
    
    if( needs_subscription )
    {
      lock l(sockets_mtx_);
      if( cfg_sub_socket_.valid() )
      {
        try
        {
          cfg_sub_socket_.get().setsockopt(ZMQ_SUBSCRIBE,
                                           name.c_str(),
                                           name.length());
        }
        catch (const std::exception & e)
        {
          std::string channel{name};
          std::string text{e.what()};
          LOG_ERROR("failed to subscribe to" << V_(channel) << "exception" << V_(text));
        }
        catch (...)
        {
          std::string channel{name};
          LOG_ERROR("failed to subscribe to" << V_(channel) << "because of an unknown exception");
        }
      }
    }
    
  }
  
  void
  config_client::remove_watches()
  {
    lock l(monitors_mtx_);
    if( !monitors_.empty() )
    {
      for( auto & m : monitors_ )
      {
        lock l(sockets_mtx_);
        if( cfg_sub_socket_.valid() )
        {
          try
          {
            cfg_sub_socket_.get().setsockopt(ZMQ_UNSUBSCRIBE,
                                             m.first.c_str(),
                                             m.first.length());
          }
          catch (const std::exception & e)
          {
            std::string channel{m.first};
            std::string text{e.what()};
            LOG_ERROR("failed to unsubscribe from" << V_(channel) << "exception" << V_(text));
          }
          catch (...)
          {
            std::string channel{m.first};
            LOG_ERROR("failed to unsubscribe from" << V_(channel) << "because of an unknown exception");
          }
        }
      }
      monitors_.clear();
    }
  }
  
  void
  config_client::remove_watches(const std::string & name)
  {
    {
      lock l(monitors_mtx_);
      auto it = monitors_.find(name);
      if( it != monitors_.end() )
      {
        {
          lock l(sockets_mtx_);
          if( cfg_sub_socket_.valid() )
          {
            try
            {
              cfg_sub_socket_.get().setsockopt(ZMQ_UNSUBSCRIBE,
                                               name.c_str(),
                                               name.length());
            }
            catch (const std::exception & e)
            {
              std::string channel{name};
              std::string text{e.what()};
              LOG_ERROR("failed to unsubscribe from" << V_(channel) << "exception" << V_(text));
            }
            catch (...)
            {
              std::string channel{name};
              LOG_ERROR("failed to unsubscribe from" << V_(channel) << "because of an unknown exception");
            }
          }
        }
        monitors_.erase(it);
      }
    }
  }
  
  config_client::~config_client()
  {
    worker_.stop();
  }
  
  endpoint_client &
  config_client::get_endpoint_client()
  {
    return *ep_client_;
  }
  
  bool
  config_client::on_endpoint_data(const interface::pb::EndpointData & ep)
  {
    bool no_change = true;
    
    std::function<bool(util::zmq_socket_wrapper & sock,
                       const std::string & addr)> connect_socket =
    {
      [this,&no_change](util::zmq_socket_wrapper & sock,
                        const std::string & addr)
      {
        bool ret = false;
        try
        {
          lock l(sockets_mtx_);
          sock.reconnect(addr.c_str());
          ret = true;
        }
        catch( const std::exception & e )
        {
          std::string text{e.what()};
          LOG_ERROR("exception in connect_socket lambda function" << V_(text));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception in connect_socket lambda function");
        }
        return ret;
      }
    };
    
    for( int i=0; i<ep.connections_size(); ++i )
    {
      auto conn = ep.connections(i);
      
      if(conn.type() == pb::ConnectionType::PUB_SUB &&
         ep.svctype() == pb::ServiceType::CONFIG)
      {
        if( !cfg_sub_socket_.connected_to(conn.address().begin(), conn.address().end()) )
        {
          for( int ii=0; ii<conn.address_size(); ++ii )
          {
            if( connect_socket(cfg_sub_socket_, conn.address(ii)) )
            {
              LOG_TRACE("config subscription configured" << V_(conn.address(ii)));              
              no_change = false;
              break;
            }
          }
        }
      } // end PUB_SUB
    }
    return no_change;
  }

}}

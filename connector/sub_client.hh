#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
#include <util/flex_alloc.hh>
#include <util/active_queue.hh>
#include <util/constants.hh>
#include <util/exception.hh>
#include "endpoint_client.hh"
#include "client_base.hh"
#include "service_type_map.hh"
#include <memory>
#include <map>

namespace virtdb { namespace connector {
  
  template <typename SUB_ITEM>
  class sub_client : public client_base
  {
  public:
    typedef SUB_ITEM                                          sub_item;
    typedef std::shared_ptr<sub_item>                         sub_item_sptr;
    typedef std::pair<std::string,sub_item_sptr>              channel_item_sptr;
  
    static const interface::pb::ConnectionType connection_type =
      interface::pb::ConnectionType::PUB_SUB;
    
    static const interface::pb::ServiceType service_type =
      service_type_map<SUB_ITEM, connection_type>::value;
    
    typedef std::function<void(const std::string & provider_name,
                               const std::string & channel,
                               const std::string & subscription,
                               sub_item_sptr data)>  sub_monitor;

  private:
    typedef std::lock_guard<std::mutex>             lock;
    typedef std::vector<sub_monitor>                monitor_vector;
    typedef std::map<std::string,monitor_vector>    monitor_map;
    
    endpoint_client                                                * ep_clnt_;
    zmq::context_t                                                   zmqctx_;
    util::zmq_socket_wrapper                                         socket_;
    util::async_worker                                               worker_;
    util::active_queue<channel_item_sptr,util::DEFAULT_TIMEOUT_MS>   queue_;
    monitor_map                                                      monitors_;
    mutable std::mutex                                               sockets_mtx_;
    mutable std::mutex                                               monitors_mtx_;
    
    bool worker_function()
    {
      try
      {
        if( !socket_.wait_valid(util::DEFAULT_TIMEOUT_MS) )
          return true;
        
        if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS) )
          return true;
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zmq::poll failed with exception" << V_(text) << "delaying subscription loop");
        lock l(sockets_mtx_);
        socket_.disconnect_all();
        return true;
      }
      
      try
      {
        // poll said we have data ...
        zmq::message_t message;
        
        if( !socket_.get().recv(&message) )
          return true;
        
        if( !message.data() || !message.size() || !message.more())
          return true;
        
        std::string subscription;
        util::zmq_socket_wrapper::valid_subscription(message, subscription);
        
        do
        {
          message.rebuild();
          if( !socket_.get().recv(&message) )
            return true;

          auto i = allocate_sub_item();
          if( i->ParseFromArray(message.data(), message.size()) )
          {
            queue_.push(std::move(std::make_pair(subscription,i)));
          }
          else
          {
            LOG_ERROR("failed to parse message" << V_(i->GetTypeName()));
          }
        }
        while( message.more() );
        
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zmq::poll failed with exception" << V_(text) << "delaying subscription loop");
        lock l(sockets_mtx_);
        socket_.disconnect_all();
        return true;
      }
      catch (const std::exception & e)
      {
        std::string exception_text{e.what()};
        LOG_ERROR("couldn't parse message" << exception_text);
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception");
      }
      return true;
    }
    
    void dispatch_function(channel_item_sptr it)
    {
      // dispatch through monitors
      lock l(monitors_mtx_);
      for( auto & m : monitors_)
      {
        if( m.first == "*" || m.first.empty() ||
            it.first.find(m.first) == 0 )
        {
          for( auto & h : m.second )
          {
            try
            {
              h(client_base::server(), it.first, m.first, it.second);
            }
            catch( const std::exception & e )
            {
              std::string text{e.what()};
              LOG_ERROR("exception thrown by subscriber function" << V_(text));
            }
          }
        }
      }
    }

  public:
    
    sub_client(endpoint_client & ep_clnt,
               const std::string & server)
    : client_base(ep_clnt, server),
      ep_clnt_(&ep_clnt),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_SUB),
      worker_(std::bind(&sub_client::worker_function, this)),
      queue_(1,std::bind(&sub_client::dispatch_function,
                         this,
                         std::placeholders::_1))
    {
      sub_item sub_itm;
      LOG_TRACE(" " << V_(sub_itm.GetTypeName()) << V_(this->server()) );
      
      // this machinery makes sure we reconnect whenever the endpoint changes
      ep_clnt.watch(service_type, [this](const interface::pb::EndpointData & ep) {
        
        bool no_change = true;
        std::string server_name = this->server();
        if( ep.name() == server_name )
        {
          LOG_TRACE("endpoint change detected for" << V_(server_name) );
          for( auto const & conn: ep.connections() )
          {
            if( conn.type() == connection_type )
            {
              if( !socket_.connected_to(conn.address().begin(),
                                        conn.address().end()) )
              {
                for( auto const & addr: conn.address() )
                {
                  try
                  {
                    LOG_INFO("reconnecting to" << V_(server_name) <<  V_(addr));
                    lock l(sockets_mtx_);
                    socket_.reconnect(addr.c_str());
                    no_change = false;
                    break;
                  }
                  catch( const std::exception & e )
                  {
                    std::string text{e.what()};
                    LOG_ERROR("exception in connect_socket lambda function" <<
                              V_(text) <<
                              V_(addr));
                  }
                  catch( ... )
                  {
                    LOG_ERROR("unknown exception in connect_socket lambda function");
                  }
                }
              }
            }
          }
        }
        return no_change;
      });
      
      worker_.start();
    }
    
    void watch(const std::string & subscription,
               sub_monitor m)
    {
      lock l(monitors_mtx_);
      auto it = monitors_.find(subscription);
      if( it == monitors_.end() )
      {
        auto rit = monitors_.insert(std::make_pair(subscription,monitor_vector()));
        it = rit.first;
      }
      it->second.push_back(m);
      if( subscription == "*" || subscription.empty() )
      {
        socket_.get().setsockopt(ZMQ_SUBSCRIBE, "*", 0);
      }
      else
      {
        socket_.get().setsockopt(ZMQ_SUBSCRIBE, subscription.c_str(), subscription.length());
      }
    }
    
    void remove_watches()
    {
      std::vector<std::string> subs;
      {
        lock l(monitors_mtx_);
        for( auto const & m : monitors_ )
          subs.push_back( m.first );
      }
      for( auto const & sub : subs )
        remove_watch(sub);
    }
    
    void remove_watch(const std::string & subscription)
    {
      lock l(monitors_mtx_);
      auto it = monitors_.find(subscription);
      if( it != monitors_.end() )
      {
        for( size_t m=0; m<it->second.size(); ++m )
        {
          try
          {
            if( it->first ==  "*" || it->first.empty() )
            {
              socket_.get().setsockopt(ZMQ_UNSUBSCRIBE, "*", 0);
            }
            else
            {
              socket_.get().setsockopt(ZMQ_UNSUBSCRIBE, it->first.c_str(), it->first.length());
            }
          }
          catch (const zmq::error_t & e)
          {
            std::string text{e.what()};
            LOG_ERROR("zmq exception" << V_(text));
          }
          catch ( const std::exception & e )
          {
            std::string text{e.what()};
            LOG_ERROR("std::exception" << V_(text));
          }
          catch( ... )
          {
            LOG_ERROR("unknown exception");
          }
        }
        monitors_.erase(it);
      }
    }
    
    virtual ~sub_client()
    {
      ep_clnt_->remove_watches(service_type);
      worker_.stop();
      queue_.stop();
    }
    
    virtual void cleanup()
    {
      ep_clnt_->remove_watches(service_type);
      worker_.stop();
      queue_.stop();
      socket_.disconnect_all();
    }
    
    virtual bool wait_valid(unsigned long ms)
    {
      return socket_.wait_valid(ms);
    }
    
    virtual void wait_valid()
    {
      socket_.wait_valid();
    }
    
    sub_item_sptr allocate_sub_item()
    {
      return allocate_sub_item_impl();
    }
    
    void release_sub_item(sub_item_sptr && i)
    {
      release_pull_item_impl(std::move(i));
    }
    
  protected:
    virtual sub_item_sptr allocate_sub_item_impl()
    {
      // this is the place to recycle pointers if really wanted
      sub_item_sptr ret{new SUB_ITEM};
      return ret;
    }
    
    virtual void release_sub_item_impl(sub_item_sptr && i)
    {
      // make sure, refcount is 0 if recycled ...
    }
    
  private:
    sub_client() = delete;
    sub_client(const sub_client & other)  = delete;
    sub_client & operator=(const sub_client &) = delete;
  };
}}


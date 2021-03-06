#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
#include <util/flex_alloc.hh>
#include <util/active_queue.hh>
#include <util/constants.hh>
#include <util/exception.hh>
#include <connector/endpoint_client.hh>
#include <connector/client_base.hh>
#include <connector/service_type_map.hh>
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
    
    struct raw_message
    {
      typedef std::shared_ptr<zmq::message_t> msg_sptr;
      std::string             subscription_;
      std::vector<msg_sptr>   messages_;
    };
    
    typedef std::shared_ptr<raw_message> raw_msg_sptr;
    
    endpoint_client                                                * ep_clnt_;
    zmq::context_t                                                   zmqctx_;
    util::zmq_socket_wrapper                                         socket_;
    util::async_worker                                               worker_;
    util::active_queue<raw_msg_sptr,util::TINY_TIMEOUT_MS>           raw_msg_queue_;
    util::active_queue<channel_item_sptr,util::DEFAULT_TIMEOUT_MS>   queue_;
    monitor_map                                                      monitors_;
    mutable std::mutex                                               sockets_mtx_;
    mutable std::mutex                                               monitors_mtx_;
    
    bool worker_function()
    {
      sub_item dbg;

      try
      {
        if( !socket_.wait_valid(util::DEFAULT_TIMEOUT_MS) )
          return true;
        
        if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS,
                             util::SHORT_TIMEOUT_MS) )
          return true;
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zmq::poll failed with exception" << V_(text) << "delaying subscription loop" <<
                  V_(dbg.GetTypeName()) <<
                  V_(this->server()));
        lock l(sockets_mtx_);
        socket_.disconnect_all();
        return true;
      }
      
      try
      {
        raw_msg_sptr raw_msg{new raw_message};
        
        // poll said we have data ...
        zmq::message_t message(0);
        
        if( !socket_.get().recv(&message) )
        {
          LOG_ERROR("failed to recv() message - while reading subscription" <<
                    V_(dbg.GetTypeName()) <<
                    V_(this->server()));
          return true;
        }
        
        if( !message.data() || !message.size() || !message.more() )
        {
          LOG_ERROR("invalid message received" <<
                    V_(dbg.GetTypeName()) <<
                    V_(this->server()) <<
                    V_(message.size()) <<
                    V_(message.more()));
          return true;
        }
        
        util::zmq_socket_wrapper::valid_subscription(message, raw_msg->subscription_);
        
        while( true )
        {
          // TODO : check this if we shall rather copy out the message from this place
          typename raw_message::msg_sptr msg_item{new zmq::message_t(0)};
          
          if( !socket_.get().recv(msg_item.get()) )
          {
            LOG_ERROR("failed to recv() message - while reading data" <<
                      V_(dbg.GetTypeName()) <<
                      V_(this->server()) <<
                      V_(raw_msg->subscription_));
            return true;
          }
          
          raw_msg->messages_.push_back(msg_item);
          
          if( !msg_item->more() ) break;
        }
        
        if( raw_msg->messages_.size() > 0 )
        {
          raw_msg_queue_.push(std::move(raw_msg));
        }
        else
        {
          LOG_ERROR("failed to construct raw message" <<
                    V_(dbg.GetTypeName()) <<
                    V_(this->server()) <<
                    V_(raw_msg->subscription_));
        }
        
      }
      catch (const zmq::error_t & e)
      {
        LOG_ERROR("zmq::poll failed with exception" << E_(e) << "delaying subscription loop" <<
                  V_(dbg.GetTypeName()) <<
                  V_(this->server()));
        lock l(sockets_mtx_);
        socket_.disconnect_all();
        return true;
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("couldn't parse message" <<
                  E_(e) <<
                  V_(dbg.GetTypeName()) <<
                  V_(this->server()));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception" <<
                  V_(dbg.GetTypeName()) <<
                  V_(this->server()));
      }
      return true;
    }
    
    void process_raw_message(raw_msg_sptr msg)
    {
      if( !msg )
      {
        LOG_ERROR("invalid message");
        return;
      }
      
      if( msg->subscription_.size() == 0 )
      {
        LOG_ERROR("subscription for message is empty" << V_(msg->messages_.size()));
        return;
      }
      
      if( msg->messages_.size() == 0 )
      {
        LOG_ERROR("empty messages arrived for" << V_(msg->subscription_));
        return;
      }
      
      for( auto & m : msg->messages_ )
      {
        auto i = sub_item_sptr{new sub_item};
        if( i->ParseFromArray(m->data(), m->size()) )
        {
          queue_.push(std::move(std::make_pair(msg->subscription_,i)));
        }
        else
        {
          LOG_ERROR("failed to parse message" <<
                    V_(i->GetTypeName()) <<
                    V_(this->server()) <<
                    V_(msg->subscription_));
        }
      }
    }
    
    void dispatch_function(channel_item_sptr it)
    {
      if( it.first.size() == 0 )
      {
        LOG_ERROR("no subscription for message");
        return;
      }
      
      if( !it.second )
      {
        LOG_ERROR("invald message arrived for" << V_(it.first));
        return;
      }
      
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
              sub_item dbg;
              LOG_ERROR("exception thrown by subscriber function" <<
                        V_(text) <<
                        V_(dbg.GetTypeName()) <<
                        V_(this->server()));
            }
          }
        }
      }
    }

  public:
    sub_client(client_context::sptr ctx,
               endpoint_client & ep_clnt,
               const std::string & server,
               size_t n_retries_on_worker_exception=10,
               bool die_on_worker_exception=true)
    : client_base(ctx,
                  ep_clnt,
                  server),
      ep_clnt_(&ep_clnt),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_SUB),
      worker_(std::bind(&sub_client::worker_function, this),
              n_retries_on_worker_exception,
              die_on_worker_exception),
      raw_msg_queue_(4,std::bind(&sub_client::process_raw_message,
                                 this,
                                 std::placeholders::_1)),
      queue_(1,std::bind(&sub_client::dispatch_function,
                         this,
                         std::placeholders::_1))
    {
      sub_item sub_itm;
      LOG_TRACE(" " << V_(sub_itm.GetTypeName()) << V_(this->server()) );
      
      // this machinery makes sure we reconnect whenever the endpoint changes
      ep_clnt.watch(service_type, [this](const interface::pb::EndpointData & ep) {
        
        std::string server_name = this->server();
        if( ep.name() == server_name )
        {
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
                    LOG_INFO("connecting to" << V_(server_name) <<  V_(addr));
                    lock l(sockets_mtx_);
                    {
                      // better to consume memory than asking for retransmission
                      zmq::socket_t & sock = socket_.get();
                      int hwm = 100000;
                      sock.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
                    }
                    socket_.reconnect(addr.c_str());
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
      socket_.disconnect_all();
      worker_.stop();
      queue_.stop();
    }
    
    virtual void rethrow_error()
    {
      worker_.rethrow_error();
    }
        
    virtual bool wait_valid(unsigned long ms)
    {
      return socket_.wait_valid(ms);
    }
    
    virtual void wait_valid()
    {
      socket_.wait_valid();
    }
      
  private:
    sub_client() = delete;
    sub_client(const sub_client & other)  = delete;
    sub_client & operator=(const sub_client &) = delete;
  };
}}


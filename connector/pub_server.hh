#pragma once

#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/flex_alloc.hh>
#include <logger.hh>
#include <memory>
#include "config_client.hh"
#include "server_base.hh"

namespace virtdb { namespace connector {
  
  template <typename ITEM>
  class pub_server : public server_base
  {
  public:
    typedef ITEM                                  pub_item;
    typedef std::shared_ptr<pub_item>             pub_item_sptr;
    
  private:
    typedef std::pair<std::string,pub_item_sptr>  to_publish;
    
    zmq::context_t                                zmqctx_;
    util::zmq_socket_wrapper                      socket_;
    util::active_queue<to_publish,15000>          queue_;
    
    void process_function(to_publish tp)
    {
      if( !tp.second ) return;
      
      try
      {
        int pub_size = tp.second->ByteSize();
        util::flex_alloc<unsigned char, 512> pub_buffer(pub_size);
        
        if( pub_size > 0 )
        {
          if( tp.second->SerializeToArray(pub_buffer.get(), pub_size) )
          {
            socket_.get().send(tp.first.c_str(), tp.first.length(), ZMQ_SNDMORE);
            socket_.get().send(pub_buffer.get(), pub_size);
            release_pub_item(std::move(tp.second));
          }
          else
          {
            LOG_ERROR("failed to serialize message");
          }
        }
      }
      catch(const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception caught");
      }
    }
    
  public:
    pub_server(config_client & cfg_client)
    : server_base(cfg_client),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_PUB),
      queue_(1,std::bind(&pub_server::process_function,
                         this,
                         std::placeholders::_1))
    {
      socket_.batch_tcp_bind(hosts());
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::PUB_SUB);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }
    
    void publish(const std::string & channel,
                 pub_item_sptr item_sptr)
    {
      queue_.push(std::make_pair(channel, item_sptr));
    }
    
    pub_item_sptr allocate_pub_item(const pub_item & i)
    {
      return allocate_pub_item_impl(i);
    }
    
    pub_item_sptr allocate_pub_item()
    {
      return allocate_pub_item_impl();
    }
    
    void release_pub_item(pub_item_sptr && i)
    {
      release_pub_item_impl(std::move(i));
    }
    
  protected:
    virtual pub_item_sptr allocate_pub_item_impl()
    {
      // this is the place to recycle pointers if really wanted
      pub_item_sptr ret{new ITEM};
      return ret;
    }

    virtual pub_item_sptr allocate_pub_item_impl(const pub_item & i)
    {
      // this is the place to recycle pointers if really wanted
      pub_item_sptr ret{new ITEM(i)};
      return ret;
    }
    
    virtual void release_pub_item_impl(pub_item_sptr && i)
    {
      // make sure, refcount is 0 if recycled ...
    }
    
  private:
    pub_server() = delete;
    pub_server(const pub_server &) = delete;
    pub_server & operator=(const pub_server &) = delete;
  };
}}

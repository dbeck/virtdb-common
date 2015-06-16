#pragma once

#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/flex_alloc.hh>
#include <util/constants.hh>
#include <logger.hh>
#include <memory>
#include "config_client.hh"
#include "server_base.hh"

namespace virtdb { namespace connector {
  
  template <typename ITEM>
  class pub_server : public server_base
  {
  public:
    typedef ITEM                                              pub_item;
    typedef std::shared_ptr<pub_item>                         pub_item_sptr;
    
  private:
    typedef std::pair<std::string,pub_item_sptr>              to_publish;
    
    zmq::context_t                                            zmqctx_;
    util::zmq_socket_wrapper                                  socket_;
    util::active_queue<to_publish,util::DEFAULT_TIMEOUT_MS>   queue_;
    
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
            if( !socket_.send(tp.first.c_str(), tp.first.length(), ZMQ_SNDMORE) )
            {
              LOG_ERROR("failed to send" << M_(*(tp.second)) << V_(tp.first));
            }
            else
            {
              if( !socket_.send(pub_buffer.get(), pub_size) )
              {
                LOG_ERROR("failed to send" << M_(*(tp.second)) << V_(tp.first));
              }
            }
          }
          else
          {
            LOG_ERROR("failed to serialize message");
          }
        }
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zeromq exception" << V_(text));
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
    pub_server(server_context::sptr ctx,
               config_client & cfg_client,
               interface::pb::ServiceType st)
    : server_base{ctx},
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_PUB),
      queue_(1,std::bind(&pub_server::process_function,
                         this,
                         std::placeholders::_1))
    {
      // save endpoint_client ref
      endpoint_client & ep_client = cfg_client.get_endpoint_client();
      
      // save endpoint_set ref
      util::zmq_socket_wrapper::endpoint_set
        ep_set{registered_endpoints(ep_client,
                                    st,
                                    interface::pb::ConnectionType::PUB_SUB)};
      {
        // better to consume memory than asking for retransmission
        zmq::socket_t & sock = socket_.get();
        int hwm = 100000;
        sock.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
      }
      
      if( !socket_.batch_ep_rebind(ep_set, true) )
      {
        socket_.batch_tcp_bind(hosts(ep_client));
      }
      else
      {
        LOG_TRACE("rebound to previous endpoint addresses");
      }
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::PUB_SUB);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }
    
    virtual void
    publish(const std::string & channel,
            pub_item_sptr item_sptr)
    {
      queue_.push(std::make_pair(channel, item_sptr));
    }
    
    virtual ~pub_server()
    {
      socket_.stop();
      queue_.stop();
    }
    
    virtual void cleanup()
    {
      socket_.disconnect_all();
      socket_.stop();
      queue_.stop();
    }
    
  private:
    pub_server() = delete;
    pub_server(const pub_server &) = delete;
    pub_server & operator=(const pub_server &) = delete;
  };
}}

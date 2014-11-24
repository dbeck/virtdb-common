#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <util/flex_alloc.hh>
#include <util/constants.hh>
#include <logger.hh>
#include "config_client.hh"
#include "server_base.hh"
#include <list>

namespace virtdb { namespace connector {
  
  template <typename REQ_ITEM,
            typename REP_ITEM>
  class rep_server : public server_base
  {
  public:
    typedef REQ_ITEM                                        req_item;
    typedef REP_ITEM                                        rep_item;
    typedef std::shared_ptr<rep_item>                       rep_item_sptr;
    typedef std::function<void(const rep_item_sptr & reply,
                               bool has_more)>              send_rep_handler;
    typedef std::function<void(const req_item & request,
                               send_rep_handler sender)>    rep_handler;
    typedef std::function<void(rep_item_sptr)>              on_reply;
    
  private:
    zmq::context_t                   zmqctx_;
    util::zmq_socket_wrapper         socket_;
    util::async_worker               worker_;
    rep_handler                      rep_handler_;
    on_reply                         on_reply_;
    
    bool worker_function()
    {
      if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS) )
        return true;

      // poll said we have data ...
      zmq::message_t message;
      
      if( !socket_.get().recv(&message) )
        return true;
      
      if( !message.data() || !message.size())
        return true;
      
      try
      {
        static req_item _req_itm;
        static rep_item _rep_itm;
        
        LOG_SCOPED("handle message" <<
                   V_(message.size()) <<
                   V_(_req_itm.GetTypeName()) <<
                   V_(_rep_itm.GetTypeName()));
        
        LOG_TRACE("start parsing message" <<
                  V_(message.size()) <<
                  V_(_req_itm.GetTypeName()));
        
        REQ_ITEM req;
        if( req.ParseFromArray(message.data(), message.size()) )
        {
          try
          {
            rep_handler_(req,[this](const rep_item_sptr & rep,
                                    bool has_more) {
              if( rep )
              {
                int rep_size = rep->ByteSize();
                util::flex_alloc<unsigned char, 1024> buffer(rep_size);
  
                if( rep->SerializeToArray(buffer.get(), rep_size) )
                {
                  if( has_more )
                  {
                    if( !socket_.send(buffer.get(), rep_size, ZMQ_SNDMORE) )
                    {
                      LOG_ERROR("failed to send" << M_(*rep));
                    }
                  }
                  else
                  {
                    if( !socket_.send(buffer.get(), rep_size) )
                    {
                      LOG_ERROR("failed to send" <<  M_(*rep));
                    }
                  }
                  
                  on_reply_(std::move(rep));
                }
                else
                {
                  LOG_ERROR("failed to serialize message");
                  if( !socket_.send("", 0) )
                  {
                    LOG_ERROR("failed to send empty message");
                  }
                }
              }
              else
              {
                if( !socket_.send("", 0) )
                {
                  LOG_ERROR("failed to send empty message");
                }
              }
            });

          }
          catch( const std::exception & e )
          {
            std::string text{e.what()};
            LOG_ERROR("exception during generating reply" << V_(text) << M_(req));
            if( !socket_.send("",0) )
            {
              LOG_ERROR("failed to send empty message");
            }
          }
        }
        else
        {
          LOG_ERROR("failed to parse message" << V_(req.GetTypeName()));
          if( !socket_.send("",0) )
          {
            LOG_ERROR("failed to send empty message");
          }
        }
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zeromq exception" << V_(text));
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("couldn't parse message. exception" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception");
      }
      
      return true;
    }

  public:
    rep_server(config_client & cfg_client,
               rep_handler handler,
               on_reply on_rep,
               size_t n_retries_on_exception=10,
               bool die_on_exception=true)
    : server_base(cfg_client),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_REP),
      worker_(std::bind(&rep_server::worker_function,
                        this),
              n_retries_on_exception,
              die_on_exception),
      rep_handler_(handler),
      on_reply_(on_rep)
    {
      socket_.batch_tcp_bind(hosts());
      worker_.start();
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::REQ_REP);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }
    
    virtual ~rep_server()
    {
      worker_.stop();
    }
    
    virtual void cleanup()
    {
      worker_.stop();
      socket_.disconnect_all();
    }
    
    virtual void rethrow_error()
    {
      worker_.rethrow_error();
    }
    
  private:
    rep_server() = delete;
    rep_server(const rep_server &) = delete;
    rep_server & operator=(const rep_server &) = delete;
  };
  
}}

#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <util/flex_alloc.hh>
#include <util/constants.hh>
#include <util/hex_util.hh>
#include <logger.hh>
#include <connector/config_client.hh>
#include <connector/server_base.hh>
#include <list>
#include <thread>

namespace virtdb { namespace connector {
  
  template <typename REQ_ITEM,
            typename REP_ITEM>
  class router_server : public server_base
  {
  public:
    typedef REQ_ITEM                                        req_item;
    typedef REP_ITEM                                        rep_item;
    typedef std::shared_ptr<req_item>                       req_item_sptr;
    typedef std::shared_ptr<rep_item>                       rep_item_sptr;
    typedef std::function<void(const rep_item_sptr & reply,
                               bool has_more)>              send_rep_handler;
    typedef std::function<void(const req_item & request,
                               send_rep_handler sender)>    rep_handler;
    typedef std::function<void(const req_item & request,
                               rep_item_sptr reply)>        on_reply;
    
  private:
    zmq::context_t                   zmqctx_;
    util::zmq_socket_wrapper         socket_;
    util::async_worker               worker_;
    rep_handler                      rep_handler_;
    on_reply                         on_reply_;
    
    bool worker_function()
    {
      static req_item _req_itm;
      static rep_item _rep_itm;
      
      if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS,
                           util::SHORT_TIMEOUT_MS) )
        return true;

      // poll said we have data ...
      zmq::message_t tmp(0);
      
      // read ID
      if( !socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive ID" << V_(_req_itm.GetTypeName()) ); return true; }
      if( !tmp.data() || !tmp.size()) { LOG_ERROR("ID has invalid content" << V_(_req_itm.GetTypeName()) << P_(tmp.data()) << V_(tmp.size()) ); return true; }
      if( !tmp.more() )               { LOG_ERROR("No more data after ID" << V_(_req_itm.GetTypeName()) << P_(tmp.data()) << V_(tmp.size()) ); return true; }
      
      // copy out data from the ZMQ message
      auto id_size = tmp.size();
      util::flex_alloc<unsigned char, 64> id(id_size);
      memcpy(id.get(), tmp.data(), id_size);
      
      // separator
      if( !socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive separator" << V_(_req_itm.GetTypeName()) ); return true; }
      if( !tmp.more() )               { LOG_ERROR("No more data after separator" << V_(_req_itm.GetTypeName()) << P_(tmp.data()) << V_(tmp.size()) ); return true; }
      
      // message
      if( !socket_.get().recv(&tmp) ) { LOG_ERROR("failed to receive message" << V_(_req_itm.GetTypeName()) ); return true; }
      if( !tmp.data() || !tmp.size()) { LOG_ERROR("message has invalid content" << V_(_req_itm.GetTypeName()) << P_(tmp.data()) << V_(tmp.size()) ); return true; }

      // copy out data from the ZMQ message
      auto msg_size = tmp.size();
      util::flex_alloc<unsigned char, 1024> msg(msg_size);
      memcpy(msg.get(),tmp.data(),msg_size);
      
      try
      {
        LOG_SCOPED("handle message" <<
                   V_(msg_size) <<
                   V_(id_size) <<
                   V_(_req_itm.GetTypeName()) <<
                   V_(_rep_itm.GetTypeName()));
        
        REQ_ITEM req;
        if( req.ParseFromArray(msg.get(), msg_size) )
        {
          try
          {
            // send ID and separator first
            socket_.send(id.get(), id_size, ZMQ_SNDMORE);
            socket_.send("", 0, ZMQ_SNDMORE);
            
            // we can send our stuff afterwards
            rep_handler_(req,[this,&req](const rep_item_sptr & rep,
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
                  
                  on_reply_(req, std::move(rep));
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
        LOG_ERROR("couldn't parse message. exception" << E_(e));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception");
      }
      
      return true;
    }

  public:
    router_server(server_context::sptr ctx,
                  config_client & cfg_client,
                  rep_handler handler,
                  on_reply on_rep,
                  interface::pb::ServiceType st)
    : server_base{ctx},
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_ROUTER),
      worker_(std::bind(&router_server::worker_function,
                        this),
              /* catch exception and ignore request.
                 users are expected to check exceptions by calling
                 rethrow_error() */
              10, false),
      rep_handler_(handler),
      on_reply_(on_rep)
    {
      // save endpoint_client ref
      endpoint_client & ep_client = cfg_client.get_endpoint_client();
      
      // save endpoint_set ref
      util::zmq_socket_wrapper::endpoint_set
      ep_set{registered_endpoints(ep_client,
                                  st,
                                  interface::pb::ConnectionType::REQ_REP)};
      
      if( !socket_.batch_ep_rebind(ep_set, true) )
      {
        socket_.batch_tcp_bind(hosts(ep_client));
      }
      else
      {
        LOG_TRACE("rebound to previous endpoint addresses");
      }
      
      worker_.start();
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::REQ_REP);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }
    
    virtual ~router_server()
    {
      socket_.stop();
      worker_.stop();
    }
    
    virtual void cleanup()
    {
      socket_.disconnect_all();
      socket_.stop();
      worker_.stop();
    }
    
    virtual void rethrow_error()
    {
      worker_.rethrow_error();
    }
    
  private:
    router_server() = delete;
    router_server(const router_server &) = delete;
    router_server & operator=(const router_server &) = delete;
  };
  
}}

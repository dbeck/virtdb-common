#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include "client_base.hh"
#include "endpoint_client.hh"
#include "service_type_map.hh"

namespace virtdb { namespace connector {
      
    template <typename REQ_ITEM,
              typename REP_ITEM>
    class req_client : public client_base
    {
    public:
      typedef REQ_ITEM req_item;
      typedef REP_ITEM rep_item;
      
      static const interface::pb::ConnectionType connection_type =
        interface::pb::ConnectionType::REQ_REP;
      
      static const interface::pb::ServiceType service_type =
        service_type_map<REQ_ITEM, connection_type>::value;
      
    private:
      typedef std::lock_guard<std::mutex> lock;
      
      endpoint_client                   * ep_clnt_;
      zmq::context_t                      zmqctx_;
      util::zmq_socket_wrapper            socket_;
      std::mutex                          mtx_;
      
    public:
      req_client(client_context::sptr ctx,
                 endpoint_client & ep_clnt,
                 const std::string & server)
      : client_base(ctx,
                    ep_clnt,
                    server),
        ep_clnt_(&ep_clnt),
        zmqctx_(1),
        socket_(zmqctx_, ZMQ_REQ)
      {
        req_item req_itm;
        rep_item rep_itm;
        LOG_TRACE(" " << V_(req_itm.GetTypeName()) << V_(rep_itm.GetTypeName()) << V_(this->server()) );
        
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
                      lock l(mtx_);
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
      }
      
       virtual bool send_request(const req_item & req,
                                 std::function<bool(const rep_item & rep)> cb,
                                 unsigned long timeout_ms,
                                 std::function<void(void)> on_timeout=[]{})
      {
        if( !socket_.valid() ) return false;
        
        zmq::message_t msg;
        int req_size = req.ByteSize();

        if( req_size )
        {
          try
          {
            util::flex_alloc<unsigned char, 1024> buffer(req_size);
            
            if( !req.SerializeToArray(buffer.get(), req_size) )
            {
              THROW_("couldn't serialize request data");
            }
            
            if( !socket_.send( buffer.get(), req_size ) )
            {
              LOG_ERROR("failed to send request" <<
                        M_(req) <<
                        V_(req.GetTypeName()) <<
                        V_(this->server()));
              return false;
            }
            
            bool call_fun = true;
            size_t n_replies = 0;
            rep_item rep;
            
            while( true )
            {
              rep.Clear();
              msg.rebuild();
              if( socket_.poll_in(timeout_ms) )
              {
                if( socket_.get().recv(&msg) )
                {
                  if( msg.data() && msg.size() )
                  {
                    if( rep.ParseFromArray(msg.data(), msg.size()) )
                    {
                      if( call_fun )
                        call_fun = cb(rep);
                      ++n_replies;
                    }
                    else
                    {
                      LOG_ERROR("failed to parse message" << V_(rep.GetTypeName()));
                    }
                  }
                }
              }
              else
              {
                if( !n_replies )
                {
                  LOG_ERROR("time out during reading reply for request" << M_(req));
                  on_timeout();
                  return false;
                }
                break;
              }
              if( !msg.more() )
                break;
            }
            return (n_replies>0);
          }
          catch (const zmq::error_t & e)
          {
            std::string text{e.what()};
            LOG_ERROR("zeromq exception" << V_(text));
          }
          catch( const std::exception & e )
          {
            std::string text{e.what()};
            LOG_ERROR("exception caught" << V_(text));
          }
          catch( ... )
          {
            LOG_ERROR("unknown exception caught");
          }
        }
        return false;
      }
      
      virtual bool wait_valid(unsigned long ms)
      {
        return socket_.wait_valid(ms);
      }

      virtual void wait_valid()
      {
        socket_.wait_valid();
      }
      
      virtual ~req_client()
      {
        ep_clnt_->remove_watches(service_type);
      }
      
      virtual void cleanup()
      {
        ep_clnt_->remove_watches(service_type);
        socket_.disconnect_all();
      }
      
    private:
      req_client() = delete;
      req_client(const req_client & other)  = delete;
      req_client & operator=(const req_client &) = delete;
    };
}}

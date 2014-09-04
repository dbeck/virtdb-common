#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include "client_base.hh"
#include "endpoint_client.hh"
#include "service_type_map.hh"
#include <future>
#include <atomic>

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
      req_client(endpoint_client & ep_clnt,
                 const std::string & server)
      : client_base(ep_clnt, server),
        ep_clnt_(&ep_clnt),
        zmqctx_(1),
        socket_(zmqctx_, ZMQ_REQ)
      {
        // this machinery makes sure we reconnect whenever the endpoint changes
        ep_clnt.watch(service_type, [this](const interface::pb::EndpointData & ep) {
          
          bool no_change = true;
          
          if( ep.name() == this->server() )
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
                      lock l(mtx_);
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
      }
      
       virtual bool send_request(const req_item & req,
                                 std::function<bool(const rep_item & rep)> cb,
                                 unsigned long timeout_ms)
      {
        if( !socket_.valid() ) return false;
        
        zmq::message_t msg;
        int req_size = req.ByteSize();

        if( req_size )
        {
          util::flex_alloc<unsigned char, 1024> buffer(req_size);
          try
          {
            if( !req.SerializeToArray(buffer.get(), req_size) )
            {
              THROW_("couldn't serialize request data");
            }
            
            socket_.get().send( buffer.get(), req_size );
            
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
                    }
                  }
                  ++n_replies;
                }
              }
              else
              {
                if( !n_replies )
                {
                  std::string req_str{req.DebugString()};
                  LOG_ERROR("time out during reading reply for request" << V_(req_str));
                  return false;
                }
                break;
              }
              if( !msg.more() )
                break;
            }
            return (n_replies>0);
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
    };
}}

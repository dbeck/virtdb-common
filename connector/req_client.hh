#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
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
#if 0
            if( !socket_.connected_to(ep.connections().begin(),
                                      ep.connections().end()) )
            {
              for( auto const & conn: ep.connections() )
              {
                if( conn.type() == connection_type )
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
#endif
          }
          return no_change;
        });
      }
      
      /*
       virtual void send_request(const interface::pb::MetaDataRequest & req,
       std::function<bool(const interface::pb::MetaData & rep)> cb);
       
       bool wait_ready(
       */
      
      virtual ~req_client()
      {
        ep_clnt_->remove_watches(service_type);
      }
    };
}}

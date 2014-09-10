#include "log_record_client.hh"
#include <logger.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include <util/constants.hh>

using namespace virtdb::interface;
using namespace virtdb::logger;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  bool log_record_client::logger_ready() const
  {
    lock l(sockets_mtx_);
    return logger_push_socket_->valid();
  }
  
  log_record_client::log_record_client(endpoint_client & ep_client,
                                       const std::string & server_name)
  : req_base_type(ep_client, server_name),
    sub_base_type(ep_client, server_name),
    zmqctx_(1),
    logger_push_socket_(new util::zmq_socket_wrapper(zmqctx_,ZMQ_PUSH)),
    log_sink_sptr_(new log_sink(logger_push_socket_))
  {
    process_info::set_app_name(ep_client.name());
    
    ep_client.watch(pb::ServiceType::LOG_RECORD,
                    [this](const pb::EndpointData & ep) {
                      return this->on_endpoint_data(ep);
                    });
  }
  
  bool
  log_record_client::on_endpoint_data(const interface::pb::EndpointData & ep)
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
      if(conn.type() == pb::ConnectionType::PUSH_PULL &&
         ep.svctype() == pb::ServiceType::LOG_RECORD)
      {
        // check if the configured address is still valid
        if( !logger_push_socket_->connected_to(conn.address().begin(), conn.address().end()) )
        {
          for( int ii=0; ii<conn.address_size(); ++ii )
          {
            if( connect_socket(*logger_push_socket_, conn.address(ii)) )
            {
              log_sink_sptr_.reset(new log_sink(logger_push_socket_));
              LOG_TRACE("logger configured" << V_(conn.address(ii)));
              no_change = false;
              break;
            }
          }
        }
      } // end PUSH_PULL
    }
    return no_change;
  }
  
  log_record_client::~log_record_client()
  {
  }
  
}}

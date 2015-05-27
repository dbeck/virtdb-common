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
  
  void log_record_client::wait_valid_push()
  {
    logger_push_socket_->wait_valid();
  }
  
  void log_record_client::wait_valid_sub()
  {
    sub_base_type::wait_valid();
  }
  
  bool log_record_client::wait_valid_push(uint64_t timeout_ms)
  {
    return logger_push_socket_->wait_valid(timeout_ms);
  }
  
  bool log_record_client::wait_valid_sub(uint64_t timeout_ms)
  {
    return sub_base_type::wait_valid(timeout_ms);
  }
  
  log_record_client::log_record_client(client_context::sptr ctx,
                                       endpoint_client & ep_client,
                                       const std::string & server_name)
  : sub_base_type(ctx,
                  ep_client,
                  server_name),
    zmqctx_(1),
    logger_push_socket_(new util::zmq_socket_wrapper(zmqctx_,ZMQ_PUSH)),
    log_sink_sptr_(new log_sink(logger_push_socket_))
  {
    process_info::set_app_name(ep_client.name());
    
    ep_client.watch(pb::ServiceType::LOG_RECORD,
                    [this](const pb::EndpointData & ep) {
                      this->on_endpoint_data(ep);
                    });
  }
  
  void
  log_record_client::on_endpoint_data(const interface::pb::EndpointData & ep)
  {
    std::function<bool(util::zmq_socket_wrapper & sock,
                       const std::string & addr)> connect_socket =
    {
      [this](util::zmq_socket_wrapper & sock,
                        const std::string & addr)
      {
        try
        {
          lock l(sockets_mtx_);
          sock.reconnect(addr.c_str());
          return true;
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
        return false;
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
              break;
            }
          }
        }
      } // end PUSH_PULL
    }
  }
  
  log_record_client::~log_record_client()
  {
  }
  
  void
  log_record_client::cleanup()
  {
    sub_base_type::cleanup();    
  }
  
  void
  log_record_client::rethrow_error()
  {
    sub_base_type::rethrow_error();
  }

}}

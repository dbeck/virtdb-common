#pragma once

#include "endpoint_client.hh"
#include <zmq.hpp>
#include <memory>
#include <logger.hh>

namespace virtdb { namespace connector {
  
  class log_record_client final
  {
    log_record_client() = delete;
    log_record_client(const log_record_client &) = delete;
    log_record_client & operator=(const log_record_client &) = delete;
    
    zmq::context_t                             zmqctx_;
    std::shared_ptr<zmq::socket_t>             logger_push_socket_sptr_;
    std::shared_ptr<virtdb::logger::log_sink>  log_sink_sptr_;
    std::string                                logger_ep_;

  public:
    log_record_client(endpoint_client & ep_client);
    ~log_record_client();
  };
}}

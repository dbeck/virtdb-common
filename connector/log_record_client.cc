#include "log_record_client.hh"
#include <logger.hh>

using namespace virtdb::interface;
using namespace virtdb::logger;

namespace virtdb { namespace connector {
  
  log_record_client::log_record_client(endpoint_client & ep_client)
  : zmqctx_(1),
    log_sink_sptr_(new log_sink(logger_push_socket_sptr_))
  {
    process_info::set_app_name(ep_client.name());
    
    ep_client.watch(pb::ServiceType::LOG_RECORD,
                    [this](const pb::EndpointData & ep) {
                      for( int i=0; i<ep.connections_size(); ++i )
                      {
                        auto conn = ep.connections(i);
                        for( int ii=0; ii<conn.address_size(); ++ii )
                        {
                          if( logger_ep_.empty() || logger_ep_ != conn.address(ii) )
                          {
                            logger_push_socket_sptr_.reset(new zmq::socket_t(zmqctx_, ZMQ_PUSH));
                            logger_push_socket_sptr_->connect(conn.address(ii).c_str());
                            if( logger_push_socket_sptr_->connected() )
                            {
                              log_sink_sptr_.reset(new log_sink(logger_push_socket_sptr_));
                              logger_ep_ = conn.address(ii);
                              LOG_INFO("logger configured" << V_(logger_ep_));
                              return false;
                            }
                          }
                        }
                      }
                      return true;
                    });
  }
  
  log_record_client::~log_record_client() {}
}}

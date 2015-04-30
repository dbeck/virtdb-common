#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "config_client.hh"
#include <logger.hh>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  config_client::config_client(client_context::sptr ctx,
                               endpoint_client & ep_client,
                               const std::string & server_name)
  : req_base_type(ctx,
                  ep_client,
                  server_name),
    sub_base_type(ctx,
                  ep_client,
                  server_name),
    ep_client_(&ep_client)
  {
  }
  
  config_client::~config_client()
  {
    this->cleanup();
  }
  
  endpoint_client &
  config_client::get_endpoint_client()
  {
    return *ep_client_;
  }
  
  void
  config_client::wait_valid_sub()
  {
    sub_base_type::wait_valid();    
  }
  
  void
  config_client::wait_valid_req()
  {
    req_base_type::wait_valid();
  }
  
  bool
  config_client::wait_valid_sub(uint64_t timeout_ms)
  {
    return sub_base_type::wait_valid(timeout_ms);
  }
  
  bool
  config_client::wait_valid_req(uint64_t timeout_ms)
  {
    return req_base_type::wait_valid(timeout_ms);
  }
  
  void
  config_client::cleanup()
  {
    req_base_type::cleanup();
    sub_base_type::cleanup();
  }
  
  void
  config_client::rethrow_error()
  {
    sub_base_type::rethrow_error();
  }

}}

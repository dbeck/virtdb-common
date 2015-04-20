#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "cert_store_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  cert_store_server::process_rep(const rep_base_type::req_item & req,
                                 rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  cert_store_server::process_req(const rep_base_type::req_item & req,
                                 rep_base_type::send_rep_handler handler)
  {
  }
  
  cert_store_server::cert_store_server(config_client & cfg_client)
  : rep_base_type(cfg_client,
                  std::bind(&cert_store_server::process_req,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&cert_store_server::process_rep,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::SRCSYS_CRED_MGR)
  
  {
  }
  
  cert_store_server::~cert_store_server()
  {
  }
  
  void
  cert_store_server::watch_requests(on_request)
  {
  }
  
  void
  cert_store_server::watch_replies(on_reply)
  {
  }
  
  void
  cert_store_server::remove_watches()
  {
  }
}}

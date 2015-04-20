#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "srcsys_credential_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  srcsys_credential_server::process_rep(const rep_base_type::req_item & req,
                                        rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  srcsys_credential_server::process_req(const rep_base_type::req_item & req,
                                        rep_base_type::send_rep_handler handler)
  {
  }

  srcsys_credential_server::srcsys_credential_server(config_client & cfg_client)
  : rep_base_type(cfg_client,
                  std::bind(&srcsys_credential_server::process_req,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&srcsys_credential_server::process_rep,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::SRCSYS_CRED_MGR)
  
  {
  }
  
  srcsys_credential_server::~srcsys_credential_server()
  {
  }
  
  void
  srcsys_credential_server::watch_requests(on_request)
  {
  }
  
  void
  srcsys_credential_server::watch_replies(on_reply)
  {
  }
  
  void
  srcsys_credential_server::remove_watches()
  {
  }
}}

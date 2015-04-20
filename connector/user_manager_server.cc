#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "user_manager_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  user_manager_server::process_rep(const rep_base_type::req_item & req,
                                   rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  user_manager_server::process_req(const rep_base_type::req_item & req,
                                   rep_base_type::send_rep_handler handler)
  {
  }
  
  user_manager_server::user_manager_server(config_client & cfg_client)
  : rep_base_type(cfg_client,
                  std::bind(&user_manager_server::process_req,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&user_manager_server::process_rep,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::SRCSYS_CRED_MGR)
  
  {
  }
  
  user_manager_server::~user_manager_server()
  {
  }
  
  void
  user_manager_server::watch_requests(on_request)
  {
  }
  
  void
  user_manager_server::watch_replies(on_reply)
  {
  }
  
  void
  user_manager_server::remove_watches()
  {
  }
}}
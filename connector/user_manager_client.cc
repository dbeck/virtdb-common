#include "user_manager_client.hh"
#include <future>

namespace virtdb { namespace connector {
  
  user_manager_client::user_manager_client(client_context::sptr ctx,
                                           endpoint_client & ep_clnt,
                                           const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  user_manager_client::~user_manager_client()
  {
  }
  
  void
  user_manager_client::cleanup()
  {
    req_base_type::cleanup();
  }
    
  bool
  user_manager_client::token_is_admin(const std::string & token,
                                      unsigned long timeout_ms)
  {
    interface::pb::UserManagerRequest req;
    req.set_type(interface::pb::UserManagerRequest::LIST_USERS);
    auto * lstreq = req.mutable_lstusers();
    lstreq->set_logintoken(token);
    interface::pb::UserManagerReply rep;
    auto fun = [&rep](const interface::pb::UserManagerReply & tmp_rep) {
      rep.MergeFrom(tmp_rep);
      return true;
    };
    bool res = this->send_request(req, fun, timeout_ms);
    if( !res ||
        rep.has_err() ||
       rep.type() != rep.LIST_USERS )
    {
      return false;
    }
    auto lsrep = rep.lstusers();
    for( auto const & i : lsrep.users() )
    {
      if( i.isadmin() )
      {
        for( auto const & ii : i.logintokens() )
        {
          if( token == ii )
          {
            return true;
          }
        }
      }
    }
    return false;
  }
  
  bool
  user_manager_client::get_srcsys_token(const std::string & input_token,
                                        const std::string & srcsys_name,
                                        interface::pb::UserManagerReply::GetSourceSysToken & result,
                                        unsigned long timeout_ms)
  {
    bool ret = false;
    interface::pb::UserManagerRequest req;
    req.set_type(interface::pb::UserManagerRequest::GET_SOURCESYS_TOKEN);
    auto * ssreq = req.mutable_getsstok();
    ssreq->set_loginortabletoken(input_token);
    ssreq->set_sourcesysname(srcsys_name);
    
    interface::pb::UserManagerReply rep;

    auto fun = [&rep](const interface::pb::UserManagerReply & tmp_rep) {
      rep.MergeFrom(tmp_rep);
      return true;
    };
    
    bool res = this->send_request(req, fun, timeout_ms);
    if( !res || rep.has_err() || rep.type() != interface::pb::UserManagerReply::GET_SOURCESYS_TOKEN )
    {
      if( rep.has_err() )
      {
        LOG_ERROR(V_(rep.err().msg()) << V_(srcsys_name));
      }
      return false;
    }

    result.CopyFrom(rep.getsstok());
    return true;
  }
  
}}

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
    if( input_token.empty() )
    {
      LOG_ERROR("empty input token");
      return false;
    }
    
    if( srcsys_name.empty() )
    {
      LOG_ERROR("empty srcsys_name");
      return false;
    }
    
    std::string itok{input_token};
    std::string sname{srcsys_name};
        
    interface::pb::UserManagerRequest req;
    req.set_type(interface::pb::UserManagerRequest::GET_SOURCESYS_TOKEN);
    auto * ssreq = req.mutable_getsstok();        
    ssreq->set_loginortabletoken(itok);
    ssreq->set_sourcesysname(sname);
    
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
        LOG_ERROR(V_(rep.err().msg()) << V_(sname));
      }
      return false;
    }

    result.CopyFrom(rep.getsstok());
    return true;
  }
  
  bool
  user_manager_client::create_login_token(const std::string & user_name,
                                          const std::string & password,
                                          std::string & result,
                                          unsigned long timeout_ms)
  {
    if( user_name.empty() )
    {
      LOG_ERROR("empty user_name");
      return false;
    }
    
    if( password.empty() )
    {
      LOG_ERROR("empty password");
      return false;
    }
    
    interface::pb::UserManagerRequest req;
    req.set_type(interface::pb::UserManagerRequest::CREATE_LOGIN_TOKEN);
    auto * ssreq = req.mutable_crlogintok();
    ssreq->set_username(user_name);
    ssreq->set_password(password);
    
    interface::pb::UserManagerReply rep;
    
    auto fun = [&rep](const interface::pb::UserManagerReply & tmp_rep) {
      rep.MergeFrom(tmp_rep);
      return true;
    };
    
    bool res = this->send_request(req, fun, timeout_ms);
    if( !res || rep.has_err() || rep.type() != interface::pb::UserManagerReply::CREATE_LOGIN_TOKEN )
    {
      if( rep.has_err() )
      {
        LOG_ERROR(V_(rep.err().msg()) << V_(user_name));
      }
      return false;
    }
    
    auto const & repmsg = rep.crlogintok();
    result = repmsg.logintoken();
    return true;
  }

  
}}

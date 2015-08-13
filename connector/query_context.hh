#pragma once

#include <data.pb.h>
#include <security.pb.h>
#include <string>
#include <memory>

namespace virtdb { namespace connector {
  
  class query_context
  {
  public:
    typedef std::shared_ptr<interface::pb::Query>                                       query_sptr;
    typedef std::shared_ptr<interface::pb::UserManagerReply::GetSourceSysToken>         srcsys_tok_reply_sptr;
    typedef std::shared_ptr<interface::pb::SourceSystemCredentialReply::GetCredential>  srcsys_cred_reply_stptr;
    
  private:
    query_context(const query_context &) = delete;
    query_context& operator=(const query_context &) = delete;
    
    query_sptr                query_sptr_;
    srcsys_tok_reply_sptr     token_sptr_;
    srcsys_cred_reply_stptr   credentials_sptr_;
    
  public:
    typedef std::shared_ptr<query_context> sptr;
    
    query_sptr query();
    srcsys_tok_reply_sptr token();
    srcsys_cred_reply_stptr credentials();

    void query(query_sptr qp);
    void token(srcsys_tok_reply_sptr tk);
    void credentials(srcsys_cred_reply_stptr crds);
    
    query_context();
    virtual ~query_context();
  };
  
}}

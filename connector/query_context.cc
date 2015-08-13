#include "query_context.hh"

namespace virtdb { namespace connector {
  
  query_context::query_context() {}
  query_context::~query_context() {}
  
  query_context::query_sptr
  query_context::query()
  {
    return query_sptr_;
  }
  
  query_context::srcsys_tok_reply_sptr
  query_context::token()
  {
    return token_sptr_;
  }
  
  query_context::srcsys_cred_reply_stptr
  query_context::credentials()
  {
    return credentials_sptr_;
  }
  
  void
  query_context::query(query_sptr qp)
  {
    query_sptr_ = qp;
  }
  
  void
  query_context::token(srcsys_tok_reply_sptr tk)
  {
    token_sptr_ = tk;
  }
  
  void
  query_context::credentials(srcsys_cred_reply_stptr crds)
  {
    credentials_sptr_ = crds;
  }
  
}}

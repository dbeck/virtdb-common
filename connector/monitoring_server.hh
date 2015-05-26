#pragma once

#include <connector/rep_server.hh>
#include <monitoring.pb.h>
#include <map>
#include <string>
#include <mutex>

namespace virtdb { namespace connector {
  
  class monitoring_server :
    public rep_server<interface::pb::MonitoringRequest,
                      interface::pb::MonitoringReply>
  {
  public:
    typedef rep_server<interface::pb::MonitoringRequest,
                       interface::pb::MonitoringReply>      rep_base_type;
    typedef std::shared_ptr<monitoring_server>              sptr;
    
  private:
    typedef std::shared_ptr<interface::pb::MonitoringReply>   common_reply_sptr;
    typedef std::lock_guard<std::mutex>                       lock;
    
    common_reply_sptr     common_reply_;
    mutable std::mutex    mtx_;
    
    void
    on_reply_fwd(const rep_base_type::req_item &,
                 rep_base_type::rep_item_sptr);
    
    void
    on_request_fwd(const rep_base_type::req_item &,
                   rep_base_type::send_rep_handler);
    
    virtual bool
    validate_token(const std::string & srcsys_token) const;
    
  protected:
    virtual void
    on_reply(const rep_base_type::req_item &,
             rep_base_type::rep_item_sptr);
    
    virtual void
    on_request(const rep_base_type::req_item &,
               rep_base_type::send_rep_handler);
    
  public:
    monitoring_server(server_context::sptr ctx,
                      config_client & cfg_client);
    virtual ~monitoring_server();
    
  };
}}

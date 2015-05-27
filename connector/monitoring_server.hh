#pragma once

#include <connector/rep_server.hh>
#include <monitoring.pb.h>
#include <list>
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
    typedef std::shared_ptr<interface::pb::MonitoringRequest::ReportStats>     report_stats_sptr;
    typedef std::shared_ptr<interface::pb::MonitoringRequest::SetState>        set_state_sptr;
    typedef std::shared_ptr<interface::pb::MonitoringRequest::ComponentError>  component_error_sptr;
    typedef std::shared_ptr<interface::pb::MonitoringRequest::RequestError>    request_error_sptr;
    
    typedef std::pair<uint64_t, report_stats_sptr>     timed_report_stats_sptr;
    typedef std::pair<uint64_t, set_state_sptr>        timed_set_state_sptr;
    typedef std::pair<uint64_t, component_error_sptr>  timed_component_error_sptr;
    typedef std::pair<uint64_t, request_error_sptr>    timed_request_error_sptr;
    
    typedef std::list<timed_report_stats_sptr>     report_stats_list;
    typedef std::list<timed_set_state_sptr>        set_state_list;
    typedef std::list<timed_component_error_sptr>  component_error_list;
    typedef std::list<timed_request_error_sptr>    request_error_list;
    
    typedef std::map<std::string, report_stats_list>      report_stats_map;
    typedef std::map<std::string, set_state_list>         set_state_map;
    typedef std::map<std::string, component_error_list>   component_error_map;
    typedef std::map<std::string, request_error_list>     request_error_map;
    
    typedef std::lock_guard<std::mutex>                       lock;
    
    report_stats_map        report_stats_map_;
    set_state_map           set_state_map_;
    component_error_map     component_error_map_;
    request_error_map       request_error_map_;
    std::set<std::string>   components_;
    mutable std::mutex      mtx_;
    
    std::pair<bool, uint64_t> locked_get_state(const std::string & name) const;
    
    void locked_add_events(interface::pb::MonitoringReply::Status & s,
                           const std::string & name) const;
    
    void
    on_reply_fwd(const rep_base_type::req_item &,
                 rep_base_type::rep_item_sptr);
    
    void
    on_request_fwd(const rep_base_type::req_item &,
                   rep_base_type::send_rep_handler);
        
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

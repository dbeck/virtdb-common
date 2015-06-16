#pragma once

#include <connector/req_client.hh>
#include <monitoring.pb.h>
#include <memory>
#include <map>
#include <mutex>

namespace virtdb { namespace connector {
  
  class monitoring_client :
    public req_client<interface::pb::MonitoringRequest,
                      interface::pb::MonitoringReply>
  {
  public:
    typedef std::shared_ptr<monitoring_client> sptr;

  private:
    typedef req_client<interface::pb::MonitoringRequest,
                       interface::pb::MonitoringReply>     req_base_type;
    
    typedef std::map<std::string, double>                  stat_map;
    typedef std::map<std::string, stat_map>                app_stats;
    typedef std::unique_lock<std::mutex>                   lock;
    
    app_stats    statistics_;
    std::mutex   mtx_;
    
    static sptr global_instance_;
    
  public:
    monitoring_client(client_context::sptr ctx,
                      endpoint_client & ep_clnt,
                      const std::string & server);
    
    bool
    report_up_down(const std::string & name,
                   bool is_up,
                   const std::string & reported_by);
    
    bool
    report_state(const std::string & name,
                 interface::pb::MonitoringRequest_SetState_Types t,
                 const char * msg = nullptr);
    
    bool
    report_bad_table_request(const std::string & name,
                             interface::pb::MonitoringRequest_RequestError_Types t,
                             const std::string & request_id,
                             const std::string & table,
                             const char * schema = nullptr,
                             const char * msg = nullptr);
    
    bool
    report_upstream_error(const std::string & impacted_peer,
                          const std::string & reported_by,
                          const char * msg = nullptr,
                          bool clear = false);
    
    void
    set_stat(const std::string & app,
             const std::string & name,
             double value);

    void
    add_stat(const std::string & app,
             const std::string & name,
             double value);
    
    bool
    send_statistics(const std::string & app);
    
    static void set_global_instance(sptr s);
    static sptr global_instance();
    
    virtual ~monitoring_client();
    virtual void cleanup();
  };
}}

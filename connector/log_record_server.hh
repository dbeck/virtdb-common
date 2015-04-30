#pragma once

#include <connector/config_client.hh>
#include <connector/pull_server.hh>
#include <connector/pub_server.hh>
#include <connector/rep_server.hh>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/compare_messages.hh>
#include <util/constants.hh>
#include <diag.pb.h>
#include <mutex>
#include <memory>
#include <tuple>

namespace virtdb { namespace connector {
  
  // TODO : integrate rep_server too ... 
  class log_record_server final :
      public pull_server<interface::pb::LogRecord>,
      public pub_server<interface::pb::LogRecord>,
      public rep_server<interface::pb::GetLogs, interface::pb::LogRecord>
  {
    typedef pull_server<interface::pb::LogRecord>              pull_base_type;
    typedef pub_server<interface::pb::LogRecord>               pub_base_type;
    typedef rep_server<interface::pb::GetLogs,
                       interface::pb::LogRecord>               rep_base_type;
    typedef interface::pb::ProcessInfo                         process_info;
    typedef interface::pb::Symbol                              symbol;
    typedef interface::pb::LogData                             log_data;
    typedef interface::pb::LogRecord                           log_record;
    typedef interface::pb::LogHeader                           log_header;
    typedef interface::pb::LogLevel                            log_level;
    typedef interface::pb::ValueType                           val_type;
    typedef std::shared_ptr<log_record>                        record_sptr;
    typedef std::shared_ptr<log_header>                        header_sptr;
    typedef std::map<uint32_t, header_sptr>                    header_map;
    typedef std::map<uint32_t, std::string>                    symbol_map;
    typedef util::compare_process_info                         comparator;
    typedef std::map<process_info, header_map, comparator>     process_headers;
    typedef std::map<process_info, symbol_map, comparator>     process_symbols;
    typedef uint64_t                                           arrived_at_usec;
    typedef std::tuple<arrived_at_usec,process_info,log_data>  log_data_item;
    typedef std::deque<log_data_item>                          log_queue;
    typedef std::lock_guard<std::mutex>                        lock;

    process_headers                         headers_;
    process_symbols                         symbols_;
    log_queue                               logs_;

    util::active_queue<record_sptr,util::DEFAULT_TIMEOUT_MS>
                                            log_process_queue_;
    std::mutex                              header_mtx_;
    std::mutex                              symbol_mtx_;
    std::mutex                              log_mtx_;

    void
    publish_log(const rep_base_type::req_item& req,
                rep_base_type::rep_item_sptr rep_sptr);
    
    void process_replies(const rep_base_type::req_item & req,
                         rep_base_type::send_rep_handler handler);
    
    bool rep_worker_function();
    void pull_handler(record_sptr);
    void process_function(record_sptr);
    void enrich_record(record_sptr);

    static const std::string & resolve(const symbol_map & smap,
                                       uint32_t id);

    static bool add_header(process_headers & destination,
                           const process_info & proc_info,
                           header_sptr hdr);
    
    void add_header(const process_info & proc_info,
                    const log_header & hdr);

    void add_symbol(const process_info & proc_info,
                    const symbol & sym);
    
    void print_data(const process_info & proc_info,
                    const log_data & data,
                    const log_header & head,
                    const symbol_map & symbol_table);
    
    void print_record( const log_record & rec );

  public:
    static const std::string & level_string(log_level level);
    static void print_variable(const val_type & var);
    
    size_t cached_log_count();
    void cleanup_older_than(uint64_t ms);
    log_record_server(server_context::sptr ctx,
                      config_client & cfg_client);
    virtual ~log_record_server();    
  };
}}

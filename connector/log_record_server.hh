#pragma once

#include "config_client.hh"
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/compare_messages.hh>
#include <util/relative_time.hh>
#include <diag.pb.h>
#include <mutex>
#include <memory>
#include <tuple>

namespace virtdb { namespace connector {
  
  class log_record_server final
  {
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

    zmq::context_t                          zmqctx_;
    util::zmq_socket_wrapper                diag_pull_socket_;
    util::zmq_socket_wrapper                diag_rep_socket_;
    util::zmq_socket_wrapper                diag_pub_socket_;
    util::async_worker                      pull_worker_;
    util::async_worker                      rep_worker_;
    util::active_queue<record_sptr,15000>   log_process_queue_;
    std::mutex                              header_mtx_;
    std::mutex                              symbol_mtx_;
    std::mutex                              log_mtx_;

    bool rep_worker_function();
    bool pull_worker_function();
    void process_function(record_sptr record);
    void enrich_record(record_sptr record);

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

    log_record_server(config_client & cfg_client);
    ~log_record_server();
    
  private:
    log_record_server() = delete;
    log_record_server(const log_record_server &) = delete;
    log_record_server & operator=(const log_record_server &) = delete;
  };
}}

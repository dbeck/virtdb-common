#pragma once

// Standard headers
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <connector/query_client.hh>
#include <connector/column_client.hh>

namespace virtdb {  namespace engine {

  class query;
  class data_handler;
  
  class receiver_thread
  {
  public:
    typedef std::shared_ptr<receiver_thread>   sptr;
    typedef std::shared_ptr<data_handler>      handler_sptr;
    
  private:
    typedef virtdb::connector::push_client<virtdb::interface::pb::Query> query_push_client;
    typedef std::unique_lock<std::mutex> lock;
    
    std::map<long, handler_sptr>  active_queries_;
    std::mutex                    mtx_;
    
    void add_query(connector::query_client::sptr query_cli,
                   connector::column_client::sptr data_client,
                   long node,
                   const virtdb::engine::query& query);
    
    receiver_thread& operator=(const receiver_thread&) = delete;
    receiver_thread(const receiver_thread&) = delete;
    
  public:
    receiver_thread();
    virtual ~receiver_thread();
    
    handler_sptr get_data_handler(long node);
    handler_sptr get_data_handler(std::string queryid);

    void
    remove_query(connector::column_client::sptr data_client,
                 long node);

    void
    send_query(connector::query_client::sptr query_cli,
               connector::column_client::sptr data_client,
               long node,
               const virtdb::engine::query& query);
    
    void
    stop_query(const std::string& table_name,
               connector::query_client::sptr cli,
               long node,
               const std::string& segment_id);
  };

  
}}

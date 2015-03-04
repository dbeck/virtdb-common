#pragma once

#include <string>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <map>

#include <engine/query.hh>
#include <engine/collector.hh>
#include <engine/feeder.hh>

namespace virtdb { namespace engine {

  class query;
  
  class data_handler {

    const std::string query_id_;
    const std::string table_name_;
    
    std::map<std::string, size_t> name_to_query_col_;
    std::map<column_id_t, size_t> column_id_to_query_col_;
    std::vector<std::string>      columns_;
    
    collector::sptr           collector_;
    feeder::sptr              feeder_;
    query::resend_function_t  resend_;
    
    data_handler& operator=(const data_handler&) = delete;
    data_handler(const data_handler&) = delete;
    
  public:
    data_handler(const query& query_data, query::resend_function_t ask_for_resend);
    virtual ~data_handler() { }
    
    // new interface
    const std::string& query_id() const;
    const std::string& table_name() const;

    void push(const std::string & name,
              std::shared_ptr<virtdb::interface::pb::Column> new_data);
    
    const std::map<column_id_t, size_t> & column_id_map() const;
    
    feeder & get_feeder();
  };
}}


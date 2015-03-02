#pragma once

#include <string>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <map>

#include <engine/chunk_store.hh>
#include <engine/data_chunk.hh>
#include <engine/collector.hh>
#include <engine/feeder.hh>

namespace virtdb { namespace engine {

  class query;
  
  class data_handler {

    std::mutex mutex;
    std::condition_variable variable;
    const std::string queryid;
    data_chunk* current_chunk = nullptr;
    chunk_store* data_store = nullptr;
    
    std::map<std::string, size_t> name_to_query_col_;
    std::map<column_id_t, size_t> column_id_to_query_col_;
    
    collector::sptr  collector_;
    feeder::sptr     feeder_;
    
    void pop_first_chunk();
    bool has_data();
    bool wait_for_data();
    bool received_data() const;
    
    data_handler& operator=(const data_handler&) = delete;
    data_handler(const data_handler&) = delete;
    
  public:
    data_handler(const query& query_data, resend_function_t ask_for_resend);
    virtual ~data_handler() { delete data_store; }
    
    bool read_next();
    bool has_more_data();
    virtdb::interface::pb::Kind get_type(int column_id);
    
    // Returns the value of the given column in the actual row or NULL
    template<typename T, interface::pb::Kind KIND = interface::pb::Kind::STRING>
    const T* const get(int column_id)
    {
      return current_chunk->get<T, KIND>(column_id);
    }
    
    const std::vector<column_id_t>& column_ids() const
    {
      return data_store->column_ids();
    }
    
    // new interface
    const std::string& query_id() const;

    void push(const std::string & name,
              std::shared_ptr<virtdb::interface::pb::Column> new_data);
    
    const std::map<column_id_t, size_t> & column_id_map() const;
    
    feeder & get_feeder();

    
  };
}}


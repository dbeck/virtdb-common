#pragma once

// protocol buffer
#include <data.pb.h>

// exceptions
#include <util/exception.hh>

// standard headers
#include <memory>

namespace virtdb {  namespace engine {

  using column_id_t = uint32_t;
  using sequence_id_t = uint64_t;
  
  class expression;
  
  class query {
    std::map<int, column_id_t> columns; // column_number -> column_id
    std::unique_ptr<virtdb::interface::pb::Query> query_data = 
      std::unique_ptr<virtdb::interface::pb::Query>(new virtdb::interface::pb::Query);
    
  public:
    typedef std::function<void(const std::vector<std::string> &, sequence_id_t)> resend_function_t;
    
    query();
    query(const std::string& id);
    query(const query& source)
    {
      *this = source;
    }
    query& operator=(const query& source)
    {
      if (this != &source)
      {
        *query_data = *source.query_data;
      }
      return *this;
    }
    
    // Id
    const ::std::string& id() const;
    
    // Table
    void set_table_name(std::string value);
    const ::std::string& table_name() const;
    
    // Columns
    void add_column(column_id_t column_id, const std::string & column);
    int columns_size() const;
    std::string column(column_id_t i) const;
    column_id_t column_id(int i) const;
    virtdb::interface::pb::Kind column_type(column_id_t i) const;
    std::string get_field(const std::string & name) const;
    std::string column_name_by_id(column_id_t id) const;
    
    // Filter
    void add_filter(std::shared_ptr<expression> filter);
    const virtdb::interface::pb::Expression& get_filter(int index) { return query_data->filter(index); }
    
    // Limit
    void set_limit(uint64_t limit);
    
    // Schema
    void set_schema(const std::string& schema);

    // User Token
    void set_usertoken(const std::string& token);
    std::string get_usertoken() const;
    
    // Accessing encapsulated object
    virtdb::interface::pb::Query& get_message();
    const virtdb::interface::pb::Query& get_message() const;
    
    bool has_segment_id() const;
    void set_segment_id(std::string segmentid);
    std::string segment_id() const ;
    
    void set_max_chunk_size(uint64_t size)
    {
      query_data->set_maxchunksize(size);
    }
  };

}}

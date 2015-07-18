#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <engine/query.hh>
#include <engine/expression.hh>
#include <engine/util.hh>
#include <logger.hh>

using namespace virtdb::interface;

namespace virtdb {  namespace engine {

  query::query() : query(gen_random(32))
  {
  }
  
  query::query(const std::string& queryid)
  {
    query_data->set_queryid(generate_hash(queryid));
  }
  
  const ::std::string&
  query::id() const
  {
    return query_data->queryid();
  }
  
  void query::set_table_name(std::string value)
  {
    query_data->set_table(value);
  }
  
  const std::string&
  query::table_name() const
  {
    return query_data->table();
  }
  
  void
  query::add_column(column_id_t column_id,
                    const std::string & col)
  {
    bool found = false;
    for (auto item : columns)
    {
      if (item.second == column_id)
      {
        found = true;
      }
    }
    if (!found)
    {
      columns[query_data->fields_size()] = column_id;
      query_data->add_fields(col);
    }
  }
  
  int query::columns_size() const
  {
    return query_data->fields_size();
  }
  
  std::string
  query::column(column_id_t i) const
  {
    return query_data->fields(i);
  }
  
  column_id_t
  query::column_id(int i) const
  {
    auto it = columns.find(i);
    if( it == columns.end() )
    {
      LOG_ERROR("invalid column id" << V_(i) << V_(columns.size()));
      THROW_("invalid column id");
    }
    else
    {
      return columns.find(i)->second;
    }
  }
  
  std::string
  query::column_name_by_id(column_id_t id) const
  {
    for (auto item : columns)
    {
      if (item.second == id)
      {
        return column(item.first);
      }
    }
    
    LOG_ERROR("not found" << V_(id) << V_(columns.size()));
    std::string err = std::string("Column not found with id: ") + std::to_string(id);
    THROW_(err.c_str());
  }
  
  void query::add_filter(std::shared_ptr<expression> filter_expression)
  {
    if( !filter_expression ) return;
    std::map<int, std::string> filter_columns = filter_expression->columns();
    for (auto it = filter_columns.begin(); it != filter_columns.end(); it++)
    {
      add_column(it->first, it->second);
    }
    *query_data->add_filter() = filter_expression->get_message();
  }
  
  void query::set_limit(uint64_t limit)
  {
    query_data->set_limit(limit);
  }
  
  void query::set_schema(const std::string& schema)
  {
    query_data->set_schema(schema);
  }

  void query::set_usertoken(const std::string& token)
  {
    query_data->set_usertoken(token);
  }
  
  pb::Query& query::get_message()
  {
    return *query_data;
  }
  
  const pb::Query& query::get_message() const
  {
    return *query_data;
  }
  
  bool query::has_segment_id() const
  {
    return query_data->has_segmentid();
  }
  
  void query::set_segment_id(std::string segmentid)
  {
    query_data->set_segmentid(segmentid);
  }
  
  std::string query::segment_id() const
  {
    return query_data->segmentid();
  }

}}

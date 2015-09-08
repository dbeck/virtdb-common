#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "data_handler.hh"
#include "query.hh"
#include <logger.hh>
#include <util/exception.hh>
#include <sstream>

namespace virtdb { namespace engine {

  data_handler::data_handler(const query& query_data,
                             query::resend_function_t ask_for_resend)
  : query_id_{query_data.id()},
    table_name_{query_data.table_name()},
    resend_{ask_for_resend}
  {
    size_t n_columns = query_data.columns_size();
    for (size_t i = 0; i < n_columns; ++i)
    {
      column_id_t col_id = query_data.column_id(i);
      std::string col_name = query_data.column(i);
      name_to_query_col_[col_name]  = i;
      column_id_to_query_col_[col_id] = i;
      columns_.push_back(col_name);
    }
    
    // initialize collector and feeder
    collector_.reset(new collector(n_columns,
                                   [this](size_t block_id, const std::vector<size_t> & cols)
                                   {
                                     std::vector<std::string> colnames;
                                     for( auto const & cn : cols )
                                     {
                                       colnames.push_back(columns_[cn]);
                                     }
                                     if( resend_ && !cols.empty() )
                                     {
                                       resend_(colnames, block_id);
                                     }
                                   }));
    
    feeder_.reset(new feeder(collector_));
    // TODO : rationalize this
    feeder_->next_block_timeout_ms(300000);
  }

  const std::map<column_id_t, size_t> &
  data_handler::column_id_map() const
  {
    return column_id_to_query_col_;
  }
  
  void
  data_handler::push(const std::string & name,
                     std::shared_ptr<virtdb::interface::pb::Column> new_data)
  {
    auto it = name_to_query_col_.find(name);
    if( it == name_to_query_col_.end() )
    {
      LOG_ERROR("cannot find column name mapping" <<
                V_(name) <<
                V_(query_id_) <<
                V_(table_name_) <<
                V_(new_data->queryid()));
      
      return;
    }
    
    LOG_TRACE(V_(name) <<
              V_(query_id()) <<
              V_(table_name()) <<
              V_(new_data->seqno()) <<
              V_(it->second) <<
              V_(new_data->seqno()) <<
              V_(new_data->endofdata()));
    
    collector_->push(new_data->seqno(),
                     it->second,
                     new_data);
  }
  
  const std::string&
  data_handler::query_id() const
  {
    return query_id_;
  }
  
  const std::string&
  data_handler::table_name() const
  {
    return table_name_;
  }
  
  feeder &
  data_handler::get_feeder()
  {
    return *feeder_;
  }
  
}} // namespace virtdb::engine

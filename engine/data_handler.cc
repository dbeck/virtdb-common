#include "data_handler.hh"
#include "query.hh"
#include <logger.hh>
#include <util/exception.hh>
#include <sstream>

namespace virtdb { namespace engine {

  data_handler::data_handler(const query& query_data, resend_function_t ask_for_resend)
  : queryid (query_data.id())
  {
    size_t n_columns = query_data.columns_size();
    for (size_t i = 0; i < n_columns; ++i)
    {
      column_id_t col_id = query_data.column_id(i);
      std::string col_name = query_data.column(i).name();
      name_to_query_col_[col_name]  = i;
      column_id_to_query_col_[col_id] = i;
    }
    
    data_store = new chunk_store(query_data, ask_for_resend);
    
    // initialize collector and feeder
    collector_.reset(new collector(n_columns));
    feeder_.reset(new feeder(collector_));
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
                V_(queryid));
      return;
    }
    
    collector_->push(new_data->seqno(),
                     it->second,
                     new_data);
    
#if 0
    bool is_complete = false;
    {
      std::unique_lock<std::mutex> cv_lock(mutex);
      data_store->push(name, new_data, is_complete);
      if( is_complete )
      {
        variable.notify_all();
      }
    }
#endif
  }
  
  const std::string&
  data_handler::query_id() const
  {
    return queryid;
  }
  
  feeder &
  data_handler::get_feeder()
  {
    return *feeder_;
  }

bool data_handler::wait_for_data()
{
    int tries = 0;
    static const int max_tries = 5;
    static const int data_timeout = 10000;
    while ( true )
    {
        // locking inside the loop so we give more chance to others to acquire the lock
        std::unique_lock<std::mutex> cv_lock(mutex);
        if( received_data() )
        {
          break;
        }

        if (variable.wait_for(cv_lock, std::chrono::milliseconds(data_timeout)) == std::cv_status::timeout)
        {
            // check that we are not missing any notifications
            if( !received_data() )
            {
                if (tries++ > max_tries)
                {
                    std::ostringstream os;
                    data_store->dump_front(os);
                    LOG_ERROR(P_(current_chunk) <<
                              V_(os.str()));
                    THROW_("Timed out while waiting for data.");
                }
                else
                {
                    LOG_INFO("Asking for missing chunks: " << V_(queryid) << V_(tries) << P_(current_chunk));
                    data_store->ask_for_missing_chunks();
                }
            }
        }
    }
    if (current_chunk == nullptr)
    {
        current_chunk = data_store->pop();
    }
    return true;
}

bool data_handler::has_more_data()
{
    if (current_chunk != nullptr and current_chunk->is_complete())
    {
        return true;
    }
    if (current_chunk == nullptr and data_store->did_pop_last())
    {
        return false;
    }
    else if (not wait_for_data())
    {
        return false;
    }
    return true;
}

virtdb::interface::pb::Kind data_handler::get_type(int column_id)
{
    return current_chunk->get_type(column_id);
}


bool data_handler::received_data() const
{
    return (current_chunk != nullptr or data_store->is_next_chunk_available());
}


bool data_handler::read_next()
{
    if (not has_more_data())
    {
        return false;
    }

    if (current_chunk != nullptr)
    {
        if (not current_chunk->read_next())
        {
            bool last = current_chunk->is_last();
            delete current_chunk;
            current_chunk = nullptr;
            if (last)
            {
                return false;
            }
        }
    }
    if (current_chunk == nullptr)
    {
        return read_next();
    }

    return true;
}

}} // namespace virtdb::engine

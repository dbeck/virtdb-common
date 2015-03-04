#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#if 0

#include "chunk_store.hh"

#include "data_chunk.hh"
#include "query.hh"
#include <logger.hh>

using namespace virtdb::engine;

void chunk_store::push(std::string name,
                       std::shared_ptr<virtdb::interface::pb::Column> new_data,
                       bool & is_complete)
{
  auto current_sequence_id = new_data->seqno();
  if( current_sequence_id > max_inserted_sequence_id )
    max_inserted_sequence_id = current_sequence_id;
  
  auto it = column_names.find(name);
  if( it == column_names.end() )
  {
    LOG_ERROR("couldn't find id for" << V_(name) << V_(column_names.size()));
    THROW_("unexpected column name");
  }
  
  auto column_id = it->second;
  
  auto * data_chunk = get_chunk(new_data->seqno());
  data_chunk->add_chunk(column_id, new_data);
  
  is_complete = data_chunk->is_complete();
  
  mark_as_received(column_id, new_data->seqno());
  
  // TODO : optimize me
  auto recvd_set = received_ids[column_id];
  auto i = max_inserted_sequence_id;
  for( i = 0; i<=max_inserted_sequence_id; ++i )
  {
    if( recvd_set.count(i) == 0 )
    {
      std::vector<std::string> cols;
      cols.push_back(name);
      ask_for_missing_chunks(cols, i);
    }
  }
}

data_chunk* chunk_store::get_chunk(sequence_id_t sequence_number)
{
    for (auto * chunk : data_container)
    {
        if (chunk->sequence_number() == sequence_number)
        {
            return chunk;
        }
    }
    for (sequence_id_t i = last_inserted_sequence_id + 1; i < sequence_number; i++)
    {
        data_container.push_back(new data_chunk(i, n_columns));
    }
    last_inserted_sequence_id = sequence_number;
    data_container.push_back(new data_chunk(sequence_number, n_columns));
    return data_container.back();
}

chunk_store::chunk_store(const query& query_data, resend_function_t _ask_for_resend)
: n_columns(query_data.columns_size()),
  ask_for_resend(_ask_for_resend),
  worker_queue(4, [](column_chunk* chunk) {
      chunk->uncompress();
  })
{
    for (int i = 0; i < n_columns; i++)
    {
        column_id_t col_id = query_data.column_id(i);
        received_ids[col_id] = std::set<sequence_id_t>();

        std::string colname = query_data.column(i).name();
        column_names[colname] = col_id;
        fields[col_id] = query_data.column(i);
        _column_ids.push_back(col_id);
    }
}

data_chunk* chunk_store::pop()
{
    data_chunk* front_chunk = nullptr;
    if (data_container.size() > 0)
    {
        front_chunk = data_container.front();

        front_chunk->uncompress(&worker_queue);

        if (front_chunk->is_last())
        {
            popped_last_chunk = true;
        }
        data_container.pop_front();
    }
    return front_chunk;
}

void chunk_store::ask_for_missing_chunks(const std::vector<std::string> & cols,
                                         sequence_id_t current_sequence_id)
{
  try
  {
    ask_for_resend(cols, current_sequence_id);
  }
  catch( const std::exception & e )
  {
    LOG_ERROR("exception caught" << E_(e));
  }
  catch( ... )
  {
    LOG_ERROR("unknown exception caught");
  }
}

void chunk_store::ask_for_missing_chunks()
{
  std::set<column_id_t> received_columns;
  if( data_container.size() > 0 )
  {
    auto * front = data_container.front();
    front->for_each([&received_columns](column_id_t id, const column_chunk &){
      received_columns.insert(id);
    });
  }

  auto i = max_inserted_sequence_id;
  for( i = 0; i<=max_inserted_sequence_id; ++i )
  {
    std::vector<std::string> cols;

    for( auto & it : column_names )
    {
      // TODO : optimize me
      auto recvd_set = received_ids[it.second];
      if( recvd_set.count(i) == 0 )
      {
        cols.push_back(it.first);
      }
    }
    
    if( cols.size()> 0 )
      ask_for_missing_chunks(cols, i);
  }
}

void chunk_store::mark_as_received(column_id_t column_id, sequence_id_t current_sequence_id)
{
    auto it = received_ids.find(column_id);
    if( it == received_ids.end() )
    {
      LOG_ERROR("not found" << V_(column_id));
      THROW_("invalid column id not found in received_ids");
    }
  
    it->second.insert(current_sequence_id);
}

bool chunk_store::is_next_chunk_available() const
{
    return (data_container.size() > 0 and data_container.front()->is_complete());
}

bool chunk_store::did_pop_last()
{
    return popped_last_chunk;
}

int chunk_store::columns_count() const
{
    return n_columns;
}

void
chunk_store::dump_front(std::ostream & os)
{
    bool front_is_complete = (data_container.size() > 0 and data_container.front()->is_complete());

    os << "data_container.size()=" << data_container.size() << " "
       << "front_is_complete=" << (front_is_complete?"TRUE":"FALSE") << " "
       << "last_inserted_sequence_id=" << last_inserted_sequence_id << " ";

    os << "front{";

    if( data_container.size() > 0 )
    {
        auto * front = data_container.front();
        std::set<column_id_t> received_columns;
        front->for_each([&received_columns](column_id_t id, const column_chunk &){
            received_columns.insert(id);
        });

        for( auto it : column_names )
        {
            os << "(" << it.first << ":";

            if( received_columns.count(it.second) == 0 )
            {
                os << "MISSING";
            }
            else
            {
                os << "OK";
            }
            os  << ')';
        }
    }
    os << "}";
}

#endif

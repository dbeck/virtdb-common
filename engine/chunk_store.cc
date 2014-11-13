#include "chunk_store.hh"

#include "data_chunk.hh"
#include "query.hh"
#include <logger.hh>

using namespace virtdb::engine;

void chunk_store::push(std::string name,
                       virtdb::interface::pb::Column* new_data,
                       bool & is_complete)
{
  auto current_sequence_id = new_data->seqno();
  LOG_SCOPED("Received new chunk." << V_(name) << V_(new_data->name()) << V_(current_sequence_id));
  
  auto it = column_names.find(name);
  if( it == column_names.end() )
  {
    LOG_ERROR("couldn't find id for" << V_(name) << V_(column_names.size()));
    THROW_("unexpected column name");
  }
  
  auto column_id = it->second;
  
  auto * data_chunk = get_chunk(new_data->seqno());
  data_chunk->add_chunk(column_id, new_data);
  
  mark_as_received(column_id, new_data->seqno());
  
  is_complete = data_chunk->is_complete();
  
  auto next_it = next_chunk.find(column_id);
  if( next_it == next_chunk.end() )
  {
    LOG_ERROR("missing" << V_(column_id) << "from next_chunk" << V_(next_chunk.size()));
    THROW_("misisng column_id");
  }
  
  for( auto i = next_it->second; i<current_sequence_id; ++i )
  {
    ask_for_missing_chunks(name, i);
  }
}

data_chunk* chunk_store::get_chunk(sequence_id_t sequence_number)
{
    LOG_SCOPED(V_(sequence_number));
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
  ask_for_resend(_ask_for_resend)
{
    for (int i = 0; i < n_columns; i++)
    {
        column_id_t col_id = query_data.column_id(i);
        next_chunk[col_id] = 0;

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

        front_chunk->uncompress();

        if (front_chunk->is_last())
        {
            popped_last_chunk = true;
        }
        data_container.pop_front();
    }
    return front_chunk;
}

void chunk_store::ask_for_missing_chunks(std::string col_name, sequence_id_t current_sequence_id)
{
  LOG_SCOPED(V_(col_name) << V_(current_sequence_id));
  
  try
  {
    ask_for_resend(col_name, current_sequence_id);
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

void chunk_store::mark_as_received(column_id_t column_id, sequence_id_t current_sequence_id)
{
    LOG_SCOPED(V_(column_id) << V_(current_sequence_id));
    if (next_chunk[column_id] == current_sequence_id)
    {
        next_chunk[column_id]++;
        LOG_TRACE("Next chunk to be waited for: " << V_(column_id) << V_(next_chunk[column_id]));
    }
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

void chunk_store::ask_for_missing_chunks()
{
    LOG_SCOPED("asking for missing");
    std::set<column_id_t> received_columns;
    if( data_container.size() > 0 )
    {
        auto * front = data_container.front();
        front->for_each([&received_columns](column_id_t id, const column_chunk &){
            received_columns.insert(id);
        });
    }

    for( auto it : column_names )
    {
        if( received_columns.count(it.second) == 0 )
        {
            ask_for_missing_chunks(it.first, next_chunk[it.second]);
        }
    }
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
            os << "(" << it.first << ":" << next_chunk[it.second] << ":";

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

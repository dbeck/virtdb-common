#include "chunk_store.hh"

#include "data_chunk.hh"
#include "query.hh"
#include <logger.hh>

using namespace virtdb::engine;

void chunk_store::push(std::string name,
                       virtdb::interface::pb::Column* new_data,
                       bool & is_complete)
{
    LOG_SCOPED("push" << V_(name));
    add(column_names[name], new_data, is_complete);
}

void chunk_store::add(int column_id,
                      virtdb::interface::pb::Column* new_data,
                      bool & is_complete)
{
    LOG_SCOPED("add" << V_(column_id));
    // if (column_id == 1)
    // {
    //     LOG_INFO("Received chunk" << V_(new_data->name()) << V_(new_data->seqno()) << V_(new_data- >endofdata()));
    // }
    LOG_TRACE("Received new chunk." << V_(column_id) << V_(new_data->name()) << V_(new_data->seqno()));
    auto * data_chunk = get_chunk(new_data->seqno());

    LOG_TRACE(P_(data_chunk) << V_(new_data->seqno()));
    data_chunk->add_chunk(column_id, new_data);

    if (not is_expected(column_id, new_data->seqno()))
    {
        ask_for_missing_chunks(column_id, new_data->seqno());
    }
    mark_as_received(column_id, new_data->seqno());

    is_complete = data_chunk->is_complete();
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
        missing_chunks[query_data.column_id(i)];
        next_chunk[query_data.column_id(i)] = 0;

        std::string colname = query_data.column(i).name();
        column_names[colname] = query_data.column_id(i);
        fields[query_data.column_id(i)] = query_data.column(i);
        _column_ids.push_back(query_data.column_id(i));
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


bool chunk_store::is_expected(column_id_t column_id, sequence_id_t current_sequence_id)
{
    if (current_sequence_id == next_chunk[column_id])
    {
        return true;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_missing_chunk);
        auto & missing_list = missing_chunks[column_id];
        if (missing_list.size() > 0 && *missing_list.begin() == current_sequence_id)
        {
            return true;
        }
    }
    LOG_TRACE("Chunk not expected: " << V_(column_id) << V_(current_sequence_id) << V_(next_chunk[column_id]));
    return false;
}

void chunk_store::ask_for_missing_chunks(column_id_t column_id, sequence_id_t current_sequence_id)
{
    LOG_SCOPED(V_(column_id) << V_(current_sequence_id));
  
    {
        std::lock_guard<std::mutex> lock(mutex_missing_chunk);
        if( missing_chunks.count(column_id) == 0 ||
            next_chunk.count(column_id) == 0 )
        {
            LOG_ERROR(V_(column_id) << "not found in missing_cunks or next_chunk");
            return;
        }
        auto & missing_list = missing_chunks[column_id];
        for (sequence_id_t i = next_chunk[column_id]; i < current_sequence_id; i++)
        {
            missing_list.push_back(i);
        }
    }
  
    timer_.schedule(2000, [this, column_id](){
        LOG_SCOPED("ask_for_missing_chunks-lambda" << V_(column_id));
        try
        {
            std::lock_guard<std::mutex> lock(mutex_missing_chunk);
            auto & missing_list = missing_chunks[column_id];
            if (missing_list.size() > 0)
            {
                ask_for_resend(column_id, missing_list);
            }
        }
        catch( const std::exception & e )
        {
            LOG_ERROR("exception caught" << E_(e));
        }
        catch( ... )
        {
            LOG_ERROR("unknown exception caught");
        }
        // telling the timer not to reschedule this function
        return false;
    });
}

void chunk_store::mark_as_received(column_id_t column_id, sequence_id_t current_sequence_id)
{
    LOG_SCOPED(V_(column_id) << V_(current_sequence_id));
    if (next_chunk[column_id] == current_sequence_id)
    {
        next_chunk[column_id]++;
        LOG_TRACE("Next chunk to be waited for: " << V_(column_id) << V_(next_chunk[column_id]));
    }
    remove_from_missing_list(column_id, current_sequence_id);
}

void chunk_store::remove_from_missing_list(column_id_t column_id, sequence_id_t current_sequence_id)
{
    std::lock_guard<std::mutex> lock(mutex_missing_chunk);
    if( missing_chunks.count(column_id) == 0 )
    {
        LOG_ERROR("missing list for" <<
                  V_(column_id) << "is not available" <<
                  V_(current_sequence_id));
        return;
    }
  
    auto & missing_list = missing_chunks[column_id];
    missing_list.remove_if([&](const sequence_id_t& item)
                           {
                               return item == current_sequence_id;
                           });
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
            ask_for_missing_chunks(it.second, next_chunk[it.second]);
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
                os << "MISSING,[";

                for( auto & missing_chunk : missing_chunks[it.second] )
                {
                    os << missing_chunk << ' ';
                }

                os << ']';
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
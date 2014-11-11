#include "data_handler.hh"
#include "query.hh"
#include <logger.hh>
#include <util/exception.hh>

namespace virtdb { namespace engine {

data_handler::data_handler(const query& query_data, std::function<void(column_id_t, const std::list<sequence_id_t>&)> ask_for_resend) :
    queryid (query_data.id())
{
    data_store = new chunk_store(query_data, ask_for_resend);
    for (int i = 0; i < data_store->columns_count(); i++)
    {
        std::string colname = query_data.column(i).name();
        column_names[colname] = query_data.column_id(i);
        fields[query_data.column_id(i)] = query_data.column(i);
        _column_ids.push_back(query_data.column_id(i));
    }
}

bool data_handler::wait_for_data()
{
    // LOG_SCOPED("wait_for_data");
    while ( true )
    {
        // locking inside the loop so we give more chance to others to acquire the lock
        std::unique_lock<std::mutex> cv_lock(mutex);
        if( received_data() )
        {
          break;
        }

        if (variable.wait_for(cv_lock, std::chrono::milliseconds(60000)) == std::cv_status::timeout)
        {
            // check that we are not missing any notifications
            if( !received_data() )
            {
                THROW_("Timed out while waiting for data.");
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

const std::string& data_handler::query_id() const
{
    return queryid;
}

void data_handler::push(std::string name, virtdb::interface::pb::Column* new_data)
{
    push(column_names[name], new_data);
}

void data_handler::push(int column_id, virtdb::interface::pb::Column* new_data)
{
    std::unique_lock<std::mutex> cv_lock(mutex);
    auto * data_chunk = data_store->add(column_id, new_data);
    if (data_chunk->is_complete())
    {
        variable.notify_all();
    }
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

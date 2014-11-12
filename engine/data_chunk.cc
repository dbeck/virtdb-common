#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "data_chunk.hh"
#include <util/exception.hh>
#include <logger.hh>
#include <utility>

using namespace virtdb::engine;

data_chunk::data_chunk(sequence_id_t _seq_no, uint32_t _n_columns)
: seq_no(_seq_no), n_columns(_n_columns)
{
}

void
data_chunk::for_each(std::function<void(column_id_t, const column_chunk &)> iterator)
{
    for( auto & it : columns )
    {
       iterator(it.first, it.second);
    }
}

int data_chunk::size()
{
    if (has_data)
    {
        return columns.begin()->second.size();
    }
    else
    {
        return 0;
    }
}

bool data_chunk::is_last()
{
    return last;
}

bool data_chunk::is_complete() const
{
    return complete;
}

void data_chunk::uncompress()
{
    for (auto& item : columns)
    {
        item.second.uncompress();
    }
}

bool data_chunk::read_next()
{
    cursor++;
    if (cursor >= size())
    {
        return false;
    }
    return true;
}

sequence_id_t data_chunk::sequence_number()
{
    return seq_no;
}

void data_chunk::add_chunk(column_id_t column_id, virtdb::interface::pb::Column* data)
{
    LOG_SCOPED(V_(column_id) << P_(data));
    if (has_data and seq_no != data->seqno())
    {
        THROW_("Trying to add chunk with different sequence number.");
    }
    has_data = true;
    columns.emplace(std::piecewise_construct,
                    std::forward_as_tuple(column_id),
                    std::forward_as_tuple(data));
    LOG_TRACE("inserted" << V_(column_id));
    // LOG_TRACE("Adding chunk" << V_(columns.size()) << V_(n_columns) << V_(seq_no) << V_(columns[column_id].size()));
    if (columns.size() == n_columns)
    {
        complete = true;
        if (data->endofdata())
        {
            last = true;
        }
    }
}

virtdb::interface::pb::Kind data_chunk::get_type(column_id_t column_id)
{
    return columns.at(column_id).get_type();
}

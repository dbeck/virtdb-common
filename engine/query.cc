#include "query.hh"
#include "expression.hh"
#include "util.hh"

using namespace virtdb::interface;

namespace virtdb {

query::query() : query(gen_random(32))
{
}

query::query(const std::string& queryid)
{
    query_data->set_queryid(generate_hash(queryid));
}

const ::std::string& query::id() const
{
    return query_data->queryid();
}

void query::set_table_name(std::string value)
{
    query_data->set_table(value);
}

const ::std::string& query::table_name() const {
    return query_data->table();
}

void query::add_column(column_id_t column_id, const pb::Field& column)
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
        pb::Field* field = query_data->add_fields();
        field->MergeFrom(column);
    }
}

// void query::add_filter(std::shared_ptr<expression> filter)
// {
//     std::map<int, std::string> filter_columns = filter->columns();
//     for (auto it = filter_columns.begin(); it != filter_columns.end(); it++)
//     {
//         add_column(it->first, it->second);
//     }
//     *query_data->add_filter() = filter->get_message();
// }

void query::set_limit(uint64_t limit)
{
    query_data->set_limit(limit);
}

void query::set_schema(const std::string& schema)
{
    query_data->set_schema(schema);
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

}

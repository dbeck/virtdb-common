// Standard headers
#include <chrono>

// Google Protocol Buffers
#include "data.pb.h"

// ZeroMQ
#include "cppzmq/zmq.hpp"

// VirtDB headers
#include <logger.hh>
#include <connector/push_client.hh>
#include <connector/sub_client.hh>
#include "receiver_thread.hh"
#include "data_handler.hh"
#include "query.hh"

using namespace virtdb::engine;
using namespace virtdb::connector;

receiver_thread::receiver_thread()
{
}

receiver_thread::~receiver_thread()
{
    for (auto it : active_queries)
    {
        delete it.second;
    }
    active_queries.empty();
}

void receiver_thread::send_query(
    push_client<interface::pb::Query>& query_client,
    sub_client<interface::pb::Column>& data_client,
    long node,
    const query& query_data
)
{
    add_query(query_client, data_client, node, query_data);
    query_client.wait_valid();
    query_client.send_request(query_data.get_message());
}

void receiver_thread::stop_query(const std::string& table_name, query_push_client& query_client, long node, const std::string& segment_id)
{
    auto * handler = active_queries[node];
    if (handler != nullptr)
    {
        virtdb::interface::pb::Query stop_query;
        stop_query.set_queryid(handler->query_id());
        stop_query.set_table(table_name);
        stop_query.set_querycontrol(virtdb::interface::pb::Query_Command_STOP);
        stop_query.set_segmentid(segment_id);
        LOG_TRACE("Sending stop query." << V_(stop_query.queryid()) << V_(stop_query.table()) << V_((int64_t)stop_query.querycontrol()));
        if( query_client.wait_valid(30000) )
        {
          query_client.send_request(stop_query);
        }
        else
        {
          LOG_ERROR("query client is not yet valid" << V_(table_name) << V_(segment_id));
        }
    }
}


void receiver_thread::add_query(
    push_client<interface::pb::Query>& query_client,
    sub_client<interface::pb::Column>& data_client,
    long node,
    const query& query_data
)
{
    {
    // for (auto i = 0; i < query_data.columns_size(); i++)
    // {
        std::string channel_id = query_data.id();
        if (query_data.has_segment_id())
        {
            channel_id +=  " " + query_data.segment_id();
        }
        LOG_TRACE("Subscribing to:" << V_(channel_id));
        data_client.watch(channel_id,
            [&](const std::string & provider_name,
                                   const std::string & channel,
                                   const std::string & subscription,
                                   std::shared_ptr<interface::pb::Column> column)
            {
                LOG_TRACE("Received column chunk." << V_(column->queryid()));
                data_handler* handler = get_data_handler(column->queryid());
                if (handler != nullptr)
                {
                    LOG_TRACE("Pushing it to handler.")
                    handler->push(column->name(), new interface::pb::Column(*column));
                }
                else
                {
                    LOG_ERROR("couldn't find a handler for" <<
                              V_(column->queryid()) <<
                              V_(column->name()) <<
                              V_(provider_name) <<
                              V_(subscription));
                }
            });
    // }
    }

    active_queries[node] = new data_handler(query_data,
        [query_data, &query_client](column_id_t column_id, const std::list<sequence_id_t>& missing_chunks)
        {
            LOG_INFO("Asking for missing chunks.");
            virtdb::interface::pb::Query new_query;
            new_query.set_queryid(query_data.id());
            new_query.set_table(query_data.table_name());
            auto * field = new_query.add_fields();
            field->set_name(query_data.column_name_by_id(column_id));
            field->mutable_desc()->set_type(query_data.column_type(column_id));
            for (auto sequence_id : missing_chunks)
            {
                LOG_TRACE(V_(sequence_id));
                new_query.add_seqnos(sequence_id);
            }
            new_query.set_querycontrol(virtdb::interface::pb::Query_Command_RESEND_CHUNK);
            // query_client.send_request(new_query);
        }
    );
}

void receiver_thread::remove_query(sub_client<interface::pb::Column>& data_client, long node)
{
    data_handler* handler = active_queries[node];
    data_client.remove_watch(handler->query_id());
    active_queries.erase(node);
    delete handler;
}

data_handler* receiver_thread::get_data_handler(long node)
{
    return active_queries[node];
}

data_handler* receiver_thread::get_data_handler(std::string queryid)
{
    for (auto it : active_queries)
    {
        if (it.second->query_id() == queryid)
        {
            return it.second;
        }
    }
    return NULL;
}

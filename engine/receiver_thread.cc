
// VirtDB headers
#include <logger.hh>
#include <connector/push_client.hh>
#include <connector/sub_client.hh>
#include <engine/receiver_thread.hh>
#include <engine/data_handler.hh>
#include <engine/query.hh>

// Standard headers
#include <chrono>
#include <vector>

using namespace virtdb::engine;
using namespace virtdb::connector;

receiver_thread::receiver_thread()
{
}

receiver_thread::~receiver_thread()
{
}

void receiver_thread::send_query(connector::query_client::sptr query_cli,
                                 connector::column_client::sptr data_client,
                                 long node,
                                 const virtdb::engine::query& query_data)
{
    add_query(query_cli, data_client, node, query_data);
    query_cli->wait_valid();  // TODO : handle timeout here
    query_cli->send_request(query_data.get_message());
    LOG_INFO("Sent query with id" << V_(query_data.id()) << V_(query_data.table_name()));
}

void
receiver_thread::stop_query(const std::string& table_name,
                            connector::query_client::sptr cli,
                            long node,
                            const std::string& segment_id)
{
  auto h = get_data_handler(node);
  if (h.get())
  {
    virtdb::interface::pb::Query stop_query;
    stop_query.set_queryid(h->query_id());
    stop_query.set_table(table_name);
    stop_query.set_querycontrol(virtdb::interface::pb::Query_Command_STOP);
    
    if( !segment_id.empty() )
      stop_query.set_segmentid(segment_id);
    
    LOG_TRACE("Sending stop query." <<
              V_(stop_query.queryid()) <<
              V_(stop_query.table()) <<
              V_((int64_t)stop_query.querycontrol()));
    
    if( cli->wait_valid(30000) )
    {
      cli->send_request(stop_query);
    }
    else
    {
      LOG_ERROR("query client is not yet valid" << V_(table_name) << V_(segment_id));
    }
  }
}


void
receiver_thread::add_query(connector::query_client::sptr query_cli,
                           connector::column_client::sptr data_client,
                           long node,
                           const query& query_data)
{
  {
    std::string query_id = query_data.id();
    std::string channel_id = query_id;
    
    if (query_data.has_segment_id())
    {
      channel_id +=  " " + query_data.segment_id();
    }
    data_client->watch(channel_id,
                      [this,query_id,node](const std::string & provider_name,
                                           const std::string & channel,
                                           const std::string & subscription,
                                           std::shared_ptr<interface::pb::Column> column)
                      {
                        if( query_id != column->queryid() )
                        {
                          LOG_ERROR("inconsistent query id" <<
                                    V_(query_id) <<
                                    V_(column->queryid()) <<
                                    V_(channel) <<
                                    V_(subscription) <<
                                    V_(column->name()) <<
                                    V_(column->seqno()) <<
                                    V_(column->endofdata()) <<
                                    V_((int64_t)node));
                          return;
                        }
                        else if( subscription.find(column->queryid()) != 0 )
                        {
                          LOG_ERROR("subscription is inconsistent w/ column's id" <<
                                    V_(query_id) <<
                                    V_(column->queryid()) <<
                                    V_(channel) <<
                                    V_(subscription) <<
                                    V_(column->name()) <<
                                    V_(column->seqno()) <<
                                    V_(column->endofdata()) <<
                                    V_((int64_t)node));
                          return;
                        }
                        
                        auto handler = get_data_handler(column->queryid());

                        if (handler.get())
                        {
                          handler->push(column->name(), column);
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

  }

  {
    lock l(mtx_);
    // LOG_TRACE("saving data handler for" << V_((int64_t)node));
    active_queries_[node] =
      handler_sptr(new data_handler(query_data,
                                    [query_data, query_cli](const std::vector<std::string> & cols,
                                                            sequence_id_t seqno)
                                    {
                                      std::ostringstream os;
                                      for( auto const & c : cols ) { os << c << " "; }
                                      LOG_INFO("Asking for missing chunks." <<
                                               V_(query_data.id()) <<
                                               V_(query_data.table_name()) <<
                                               V_(cols.size()) <<
                                               V_(os.str()) <<
                                               V_(seqno) <<
                                               V_(query_data.segment_id()));
                                      
                                      virtdb::interface::pb::Query new_query;
                                      new_query.set_queryid(query_data.id());
                                      new_query.set_table(query_data.table_name());
                                      new_query.set_segmentid(query_data.segment_id());
                                      for( auto const & colname : cols )
                                      {
                                        new_query.add_fields(colname);
                                      }
                                      new_query.add_seqnos(seqno);
                                      new_query.set_querycontrol(virtdb::interface::pb::Query_Command_RESEND_CHUNK);
                                      query_cli->send_request(new_query);
                                    }
                                    ));
  }
}

void
receiver_thread::remove_query(connector::column_client::sptr data_client,
                              long node)
{
  lock l(mtx_);
  auto it = active_queries_.find(node);
  if( it == active_queries_.end() )
    return;
  
  data_client->remove_watch(it->second->query_id());
  active_queries_.erase(it);
}

receiver_thread::handler_sptr
receiver_thread::get_data_handler(long node)
{
  handler_sptr ret;
  {
    lock l(mtx_);
    auto it = active_queries_.find(node);
    if( it != active_queries_.end() )
      ret = it->second;
  }
  return ret;
}

receiver_thread::handler_sptr
receiver_thread::get_data_handler(std::string queryid)
{
  handler_sptr ret;
  {
    lock l(mtx_);
    for (auto it : active_queries_)
    {
      if (it.second->query_id() == queryid)
      {
        ret = it.second;
        return ret;
      }
    }
  }
  return ret;
}

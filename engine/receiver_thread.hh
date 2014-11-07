// Standard headers
#include <string>
#include <map>

namespace gpdb {
    unsigned long UlSegmentCountGP();
}


namespace virtdb
{
    namespace connector
    {
        template<typename PUSH_ITEM> class push_client;
        template<typename SUB_ITEM> class sub_client;
    }
    class query;
    class data_handler;
}

namespace virtdb {
    class receiver_thread
    {
        typedef virtdb::connector::push_client<virtdb::interface::pb::Query> query_push_client;

        private:
            std::map<long, data_handler* > active_queries;

            void add_query(
                query_push_client& query_client,
                virtdb::connector::sub_client<virtdb::interface::pb::Column>& data_client,
                long node,
                const virtdb::query& query
            );

            receiver_thread& operator=(const receiver_thread&) = delete;
            receiver_thread(const receiver_thread&) = delete;

        public:
            receiver_thread();
            virtual ~receiver_thread();

            void remove_query(virtdb::connector::sub_client<virtdb::interface::pb::Column>& data_client, long node);
            data_handler* get_data_handler(long node);
            data_handler* get_data_handler(std::string queryid);
            void send_query(
                query_push_client& query_client,
                virtdb::connector::sub_client<virtdb::interface::pb::Column>& data_client,
                long node,
                const virtdb::query& query
            );

            void stop_query(const std::string& table_name, query_push_client& query_client, long node, const std::string& segment_id);
    };
}

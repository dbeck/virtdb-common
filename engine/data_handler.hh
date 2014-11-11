#pragma once

#include <string>
#include <vector>
#include <condition_variable>
#include <mutex>

#include "chunk_store.hh"
#include "data_chunk.hh"

namespace virtdb { namespace engine {

    class query;

    class data_handler {
        private:
            std::mutex mutex;
            std::condition_variable variable;
            const std::string queryid;
            data_chunk* current_chunk = nullptr;
            chunk_store* data_store = nullptr;

            void pop_first_chunk();
            bool has_data();
            bool wait_for_data();
            bool received_data() const;

            data_handler& operator=(const data_handler&) = delete;
            data_handler(const data_handler&) = delete;

        public:
            data_handler(const query& query_data, std::function<void(column_id_t, const std::list<sequence_id_t>&)> ask_for_resend);
            virtual ~data_handler() { delete data_store; }
            const std::string& query_id() const;
            void push(std::string name, virtdb::interface::pb::Column* new_data);
            bool read_next();
            bool has_more_data();
            virtdb::interface::pb::Kind get_type(int column_id);

            // Returns the value of the given column in the actual row or NULL
            template<typename T, interface::pb::Kind KIND = interface::pb::Kind::STRING>
            const T* const get(int column_id)
            {
                return current_chunk->get<T, KIND>(column_id);
            }
      
            const std::vector<column_id_t>& column_ids() const
            {
                return data_store->column_ids();
            }
    };
}}


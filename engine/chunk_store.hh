#pragma once

#include <list>
#include <mutex>
#include <stdint.h>
#include <iosfwd>
#include <set>
#include "data_chunk.hh"
#include "column_chunk.hh"
#include <util/active_queue.hh>

namespace virtdb { namespace engine {

    class query;

    typedef std::function<void(std::string, sequence_id_t)> resend_function_t;

    class chunk_store {
        private:
            const int n_columns;
            std::list<data_chunk*> data_container;
            bool popped_last_chunk = false;
            resend_function_t ask_for_resend;
            sequence_id_t last_inserted_sequence_id = 0;
            sequence_id_t max_inserted_sequence_id = 0;
            virtdb::util::active_queue<column_chunk*, 20> worker_queue;
      
            std::map<std::string, column_id_t>  column_names;
            std::map<column_id_t, virtdb::interface::pb::Field> fields;
            std::map<column_id_t, std::set<sequence_id_t>> received_ids;
            std::vector<column_id_t> _column_ids;
      
      
            data_chunk* get_chunk(sequence_id_t sequence_number);
            void ask_for_missing_chunks(std::string, sequence_id_t);
            void mark_as_received(column_id_t, sequence_id_t);

        public:
            chunk_store(const query& query_data, resend_function_t _ask_for_resend);
            data_chunk* pop();

            bool is_next_chunk_available() const;
            bool did_pop_last();
            int columns_count() const;

            inline const std::vector<column_id_t>& column_ids() const
            {
                return _column_ids;
            }

            void push(std::string name,
                      virtdb::interface::pb::Column* new_data,
                      bool & is_complete);
            void dump_front(std::ostream & os);

            void ask_for_missing_chunks();
    };
}}

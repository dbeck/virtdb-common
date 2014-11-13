#pragma once

#include <list>
#include <mutex>
#include <stdint.h>
#include <iosfwd>
#include <util/timer_service.hh>
#include "data_chunk.hh"

namespace virtdb { namespace engine {

    class query;

    typedef std::function<void(column_id_t, const std::list<sequence_id_t>&)> resend_function_t;

    class chunk_store {
        private:
            std::mutex mutex_missing_chunk;
            const int n_columns;
            std::list<data_chunk*> data_container;
            bool popped_last_chunk = false;
            std::map<column_id_t, std::list<sequence_id_t>> missing_chunks;
            std::map<column_id_t, sequence_id_t> next_chunk;
            resend_function_t ask_for_resend;
            sequence_id_t last_inserted_sequence_id = 0;

            std::map<std::string, column_id_t>  column_names;
            std::map<column_id_t, virtdb::interface::pb::Field> fields;
            std::vector<column_id_t> _column_ids;
      
            util::timer_service timer_;

            data_chunk* get_chunk(sequence_id_t sequence_number);
            bool is_expected(column_id_t, sequence_id_t);
            void ask_for_missing_chunks(column_id_t, sequence_id_t);
            void mark_as_received(column_id_t, sequence_id_t);
            void remove_from_missing_list(column_id_t, sequence_id_t);

            void add(int column_id,
                     virtdb::interface::pb::Column* new_data,
                     bool & is_complete);

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

#pragma once

#include <map>
#include "column_chunk.hh"
#include <common.pb.h>
#include "query.hh"

namespace virtdb {

class data_chunk {
    private:
        bool has_data = false;
        bool last = false;
        bool complete = false;
        sequence_id_t seq_no = 0;
        std::map<column_id_t, column_chunk> columns;
        uint32_t n_columns = 0;
        int cursor = -1;
    public:
        data_chunk(sequence_id_t _seq_no, uint32_t _column_count);
        int size();
        bool is_last();
        bool is_complete() const;
        void uncompress();
        bool read_next();
        sequence_id_t sequence_number();
        void add_chunk(column_id_t column_id, virtdb::interface::pb::Column* data);
        virtdb::interface::pb::Kind get_type(column_id_t column_id);

        template<typename T, interface::pb::Kind KIND = interface::pb::Kind::STRING>
        const T* const get(int column_id)
        {
            return columns[column_id].get<T, KIND>(cursor);
        }


};

}

#pragma once

#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

// protocol buffer
#include <data.pb.h>

// exceptions
#include <util/exception.hh>

// standard headers
#include <memory>

namespace virtdb {
    using column_id_t = uint32_t;
    using sequence_id_t = uint64_t;

    class expression;

    class query {
        private:
            std::map<int, column_id_t> columns; // column_number -> column_id
            std::unique_ptr<virtdb::interface::pb::Query> query_data =
                    std::unique_ptr<virtdb::interface::pb::Query>(new virtdb::interface::pb::Query);

        public:
            query();
            query(const std::string& id);
            query(const query& source)
            {
                *this = source;
            }
            query& operator=(const query& source)
            {
                if (this != &source)
                {
                    *query_data = *source.query_data;
                }
                return *this;
            }

            // Id
            const ::std::string& id() const;

            // Table
            void set_table_name(std::string value);
            const ::std::string& table_name() const;

            // Columns
            void add_column(column_id_t column_id, const virtdb::interface::pb::Field& column);
            int columns_size() const { return query_data->fields_size(); }
            virtdb::interface::pb::Field column(column_id_t i) const { return query_data->fields(i); }
            column_id_t column_id(int i) const { return columns.find(i)->second; }
            virtdb::interface::pb::Kind column_type(column_id_t i) const { return query_data->fields(i).desc().type(); }
            std::string column_name_by_id(column_id_t id) const
            {
                for (auto item : columns)
                {
                    if (item.second == id)
                    {
                        return column(item.first).name();
                    }
                }

                std::string err = std::string("Column not found with id: ") + std::to_string(id);
                THROW_(err.c_str());
            }

            // Filter
            void add_filter(std::shared_ptr<expression> filter);
            const virtdb::interface::pb::Expression& get_filter(int index) { return query_data->filter(index); }

            // Limit
            void set_limit(uint64_t limit);

            // Schema
            void set_schema(const std::string& schema);

            // Accessing encapsulated object
            virtdb::interface::pb::Query& get_message();
            const virtdb::interface::pb::Query& get_message() const;

            bool has_segment_id() const;
            void set_segment_id(std::string segmentid);
            std::string segment_id() const ;

            void set_max_chunk_size(uint64_t size)
            {
                query_data->set_maxchunksize(size);
            }
    };
}

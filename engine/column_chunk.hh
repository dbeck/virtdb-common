#pragma once

#include <data.pb.h>
#include <util/value_type.hh>
#include <util/exception.hh>
#include <sstream>

namespace virtdb {  namespace engine {

    class column_chunk {
        private:
            virtdb::interface::pb::Column* column_data = nullptr;
        public:
            virtual ~column_chunk();
            int size();
            void uncompress();
            void operator=(virtdb::interface::pb::Column* right_operand);
            void operator=(const column_chunk& source);
            virtdb::interface::pb::Kind get_type();

            template<typename T, interface::pb::Kind KIND = interface::pb::Kind::STRING>
            const T* const get(int cursor)
            {
                try {
                    auto * value = column_data->mutable_data();
                    if (virtdb::util::value_type<T>::is_null(*value, cursor))
                        return NULL;
                    return &virtdb::util::value_type<T, KIND>::get(*value, cursor);
                }
                catch(const ::google::protobuf::FatalException & e)
                {
                    std::ostringstream os;
                    os << "Error [" << __LINE__ <<  "]! Inner_cursor: " << cursor << " " << e.what();
                    THROW_(os.str().c_str());
                }
            }
      
            const std::string & name() const;
    };

}}

#include "engine_test.hh"
#include <engine/column_chunk.hh>

using namespace virtdb::engine;
using namespace virtdb::test;
using namespace virtdb::interface::pb;

class DummyColumn : public Column
{
public:
    DummyColumn(const std::string & name = "", Kind type = Kind::STRING, size_t records = 0)
    {
        set_name(name);
        mutable_data()->set_type(type);
        for (size_t i = 0; i < records; i++)
        {
            mutable_data()->add_stringvalue("asd");
        }
    }
};

TEST_F(ColumnChunkTest, empty)
{
    DummyColumn* column = new DummyColumn();
    column_chunk chunk(column);

    ASSERT_EQ(0, chunk.size());
    ASSERT_NO_THROW(chunk.uncompress());
    ASSERT_NO_THROW(chunk.get_type());
    ASSERT_EQ(Kind::STRING, chunk.get_type());
    ASSERT_THROW(chunk.get<std::string>(0), std::exception);
    ASSERT_EQ("", chunk.name());
}

TEST_F(ColumnChunkTest, empty_int32_withname)
{
    auto name = "Colname";
    auto type = Kind::INT32;
    DummyColumn* column = new DummyColumn(name, type, 0);
    column_chunk chunk(column);

    ASSERT_EQ(0, chunk.size());
    ASSERT_NO_THROW(chunk.uncompress());
    ASSERT_NO_THROW(chunk.get_type());
    ASSERT_EQ(type, chunk.get_type());
    ASSERT_THROW(chunk.get<std::string>(0), std::exception);
    ASSERT_EQ(name, chunk.name());
}

#include "engine_test.hh"
#include <engine/data_handler.hh>
#include <engine/expression.hh>
#include <engine/query.hh>
#include <engine/receiver_thread.hh>
#include <engine/util.hh>
#include <engine/collector.hh>
#include <engine/feeder.hh>

using namespace virtdb::util;
using namespace virtdb::engine;
using namespace virtdb::test;
using namespace virtdb::interface::pb;

// check invalid sequence number
// check invalid query id in the column

class DummyColumn : public Column
{
public:
    typedef std::shared_ptr<DummyColumn> sptr;
  
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


#include "column_data.hh"
#include <cachedb/column_hasher.hh>
#include <util/exception.hh>
#include <util/hex_util.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  column_data::clazz() const
  {
    static std::string clz("column_data");
    return clz;
  }
  
  size_t
  column_data::key_len() const
  {
    return 16;
  }
  
  void
  column_data::default_columns()
  {
    column("data");
  }
  
  column_data::column_data() : storeable(), len_{0} {}
  column_data::~column_data() {}
  
  void
  column_data::set(const interface::pb::Column & c)
  {
    THROW_("implement me");
  }
  
  void
  column_data::reset()
  {
    THROW_("implement me");
  }
}}

#include "column_data.hh"
#include <util/exception.hh>

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
  
  column_data::column_data() : storeable() {}
  column_data::~column_data() {}
  
}}

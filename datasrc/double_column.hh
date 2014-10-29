#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {

  class double_column : public typed_column<double>
  {
  public:
    double_column(size_t max_rows, on_dispose d=[](){});
  };
  
}}

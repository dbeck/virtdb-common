#include "float_column.hh"
#include <util/value_type.hh>

namespace virtdb { namespace datasrc {
  
  float_column::float_column(size_t max_rows)
  : parent_type{max_rows}
  {
  }
  
  void
  float_column::convert_pb()
  {
    size_t n = std::min(max_rows(), n_rows());
    auto & column_pb = get_pb_column();
    auto * data_pb_ptr = column_pb.mutable_data();
    
    data_pb_ptr->Clear();
    data_pb_ptr->set_type(interface::pb::Kind::FLOAT);
    auto * mdv = data_pb_ptr->mutable_floatvalue();
    mdv->Reserve(n);
    
    auto * nulls_pb = data_pb_ptr->mutable_isnull();
    nulls_pb->Clear();
    nulls_pb->Reserve(n);
    
    auto & null_vals = this->nulls();
    auto * values = get_typed_ptr();
    
    for( size_t i=0; i<n; ++i  )
    {
      if( null_vals[i] )
      {
        data_pb_ptr->add_floatvalue(0.0);
        util::value_type_base::set_null(*data_pb_ptr, i);
      }
      else
      {
        data_pb_ptr->add_floatvalue(values[i]);
      }
    }
  }
}}

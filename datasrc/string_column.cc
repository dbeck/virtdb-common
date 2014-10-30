#include "string_column.hh"
#include <util/value_type.hh>

namespace virtdb { namespace datasrc {
  
  string_column::string_column(size_t max_rows,
                               size_t max_size)
  : parent_type{max_rows, max_size}
  {
  }
  
  void
  string_column::convert_pb()
  {
    size_t n = std::min(max_rows(), n_rows());
    auto & column_pb = get_pb_column();
    auto * data_pb_ptr = column_pb.mutable_data();
    
    data_pb_ptr->Clear();
    data_pb_ptr->set_type(interface::pb::Kind::STRING);
    auto * mdv = data_pb_ptr->mutable_stringvalue();
    mdv->Reserve(n);
    
    auto * nulls_pb = data_pb_ptr->mutable_isnull();
    nulls_pb->Clear();
    nulls_pb->Reserve(n);
    
    auto & null_vals = this->nulls();
    auto * val_ptr = get_ptr();
    auto & sizes = actual_sizes();
    
    for( size_t i=0; i<n; ++i  )
    {
      if( null_vals[i] )
      {
        data_pb_ptr->add_stringvalue(val_ptr+in_field_offset(), sizes[i]);
        util::value_type_base::set_null(*data_pb_ptr, i);        
      }
      else
      {
        data_pb_ptr->add_stringvalue("",0);
      }
      val_ptr += max_size();
    }
  }
  
}}

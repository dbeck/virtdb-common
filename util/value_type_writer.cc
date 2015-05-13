#include <util/value_type_writer.hh>
#include <util/exception.hh>

namespace virtdb { namespace util {
  
  value_type_writer::value_type_writer(interface::pb::Kind kind,
                                       size_t initial_mempool_size,
                                       size_t estimated_item_count)
  : mpool_(initial_mempool_size),
    root_{0,0,0,0},
    nulls_{0,0,0,0},
    estimated_item_count_{estimated_item_count},
    last_null_{0},
    null_part_chain_{nullptr},
    null_part_{nullptr},
    null_offset_{0}
  {
    using namespace google::protobuf::io;
    
    root_.next_       = &nulls_;
    null_part_chain_  = &nulls_;
    nulls_.allocate(mpool_,8);
    nulls_.parts_[0].allocate(mpool_, 16);
    nulls_.parts_[1].allocate(mpool_, estimated_item_count);
    nulls_.n_parts_ = 2;
    ::memset(nulls_.parts_[1].head_,0,estimated_item_count);
    null_part_ = nulls_.parts_+1;
    // set tag
    ::memset(nulls_.parts_[0].head_,0,nulls_.parts_[0].n_allocated_);
    CodedOutputStream::WriteTagToArray((11<<3)+2, nulls_.parts_[0].head_);
  }
  
  value_type_writer::sptr
  value_type_writer::construct(interface::pb::Kind kind,
                               size_t estimated_item_count)
  {
    value_type_writer::sptr ret;
    
    switch( kind )
    {
      case interface::pb::Kind::STRING:
      case interface::pb::Kind::DATETIME:
      case interface::pb::Kind::NUMERIC:
      case interface::pb::Kind::INET4:
      case interface::pb::Kind::INET6:
      case interface::pb::Kind::MAC:
      case interface::pb::Kind::GEODATA:
        ret.reset(new vtw_impl::string_writer(estimated_item_count));
        break;

      case interface::pb::Kind::DATE:
        ret.reset(new vtw_impl::date_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::TIME:
        ret.reset(new vtw_impl::time_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::INT32:
        ret.reset(new vtw_impl::int32_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::INT64:
        ret.reset(new vtw_impl::int64_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::BOOL:
        ret.reset(new vtw_impl::bool_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::UINT32:
        ret.reset(new vtw_impl::uint32_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::UINT64:
        ret.reset(new vtw_impl::uint64_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::DOUBLE:
        ret.reset(new vtw_impl::double_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::FLOAT:
        ret.reset(new vtw_impl::float_writer(estimated_item_count));
        break;
        
      case interface::pb::Kind::BYTES:
        ret.reset(new vtw_impl::bytes_writer(estimated_item_count));
        break;
    };
    
    return ret;
  }
}}

#include "column_data.hh"
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
  
  column_data::column_data()
  : storeable(),
    len_{0}
  {
  }
  
  column_data::~column_data() {}
  
  void
  column_data::set(const interface::pb::Column & c)
  {
    interface::pb::Column tmp;
    
    tmp.mutable_data()->MergeFrom(c.data());
    
    if( c.has_comptype() )
      tmp.set_comptype(c.comptype());
    
    if( c.has_compresseddata() )
      *tmp.mutable_compresseddata() = c.compresseddata();
    
    if( c.has_uncompressedsize() )
      tmp.set_uncompressedsize(c.uncompressedsize());

    int byte_size = tmp.ByteSize();
    if( byte_size <= 0  ) { THROW_("empty object"); }
    
    if( byte_size >= len_ )
    {
      // this should be the state if fail to allocate
      len_       = 0;
      
      // clear parent
      storeable::clear();

      data_.reset(new uint8_t[byte_size]);
      len_ = byte_size;
    }
    
    tmp.SerializePartialToArray(data_.get(), byte_size);
    //
    {
      
    }

  }
  
  void
  column_data::clear()
  {
    // clear parent
    storeable::clear();
    
    // clear own data
    data_.reset();
    len_        = 0;
  }
  
  size_t
  column_data::len() const
  {
    return len_;
  }

}}

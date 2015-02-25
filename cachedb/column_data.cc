#include "column_data.hh"
#include <util/exception.hh>
#include <util/hex_util.hh>
#include <cachedb/hash_util.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name column_data::qn_data{column_data::clazz_static(),"data"};
  
  const std::string &
  column_data::clazz_static()
  {
    static std::string clz("column_data");
    return clz;
  }
  
  const std::string &
  column_data::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  column_data::key_len() const
  {
    return 16;
  }
  
  void
  column_data::default_columns()
  {
    column(qn_data);
  }
  
  column_data::column_data()
  : storeable()
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
    
    if( !tmp.SerializePartialToString(&data_) )
    {
      LOG_ERROR("failed to serialize data"
                << V_(c.queryid())
                << V_(c.name())
                << V_(c.seqno()));
      THROW_("failed to serialize data");
    }
    
    {
      // generate hash
      std::string hash_val;
      if( hash_util::hash_data(data_.data(), data_.size(), hash_val) )
        this->key(hash_val);
      else
      {
        LOG_ERROR("failed to generate hash value"
                  << V_(c.queryid())
                  << V_(c.name())
                  << V_(c.seqno()));
        THROW_("cannot generate hash value");
      }
      
      // set property
      this->property(qn_data, data_);
    }
  }
  
  void
  column_data::clear()
  {
    // clear parent
    storeable::clear();
    
    // clear own data
    data_.erase();
  }
  
  size_t
  column_data::len() const
  {
    return data_.size();
  }

}}

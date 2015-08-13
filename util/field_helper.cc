#include <util/field_helper.hh>

namespace virtdb { namespace util {
  
  field_helper::field_helper()
  {
  }
  
  field_helper::~field_helper()
  {
  }
  
  void
  field_helper::init(const interface::pb::Field & field)
  {
    propery_map tmp;
    
    for( auto & f : field.properties() )
      tmp[f.key()] = f.value();
    
    tmp.swap(properties_);
  }
  
  void
  field_helper::save(interface::pb::Field & field) const
  {
    field.clear_properties();
    
    for( auto const & p : properties_ )
    {
      interface::pb::KeyValue kv;
      kv.set_key(p.first);
      kv.mutable_value()->MergeFrom(p.second);
      field.add_properties()->MergeFrom(kv);
    }
  }
  
  void
  field_helper::remove_property(const std::string & name)
  {
    auto it = properties_.find(name);
    if( it != properties_.end() )
      properties_.erase(it);
  }
  
}}

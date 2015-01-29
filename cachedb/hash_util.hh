#pragma once

#include <data.pb.h>
#include <meta_data.pb.h>

namespace virtdb { namespace cachedb {
  
  class hash_util final
  {
  public:
    static void hex(unsigned long long in,
                    std::string & out);
    
    static bool hash_string(const std::string in,
                            std::string & out);
    
    static bool hash_query(const interface::pb::Query & in,
                           std::string & out);
    
    static bool hash_field(const interface::pb::Query & q_in,
                           const interface::pb::Field & field_in,
                           std::string & out);
  };
  
}}
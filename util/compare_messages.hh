#pragma once

#include <svc_config.pb.h>

namespace virtdb { namespace util {
  
  struct compare_endpoint_data
  {
    bool operator()(const interface::pb::EndpointData & lhs,
                    const interface::pb::EndpointData & rhs) const
    {
      if( lhs.name() < rhs.name() )
        return true;
      else if (lhs.name() > rhs.name() )
        return false;
      
      if( lhs.svctype() < rhs.svctype() )
        return true;
      else
        return false;
    }
  };

}}

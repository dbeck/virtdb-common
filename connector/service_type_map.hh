#pragma once

#include <svc_config.pb.h>
#include <meta_data.pb.h>

namespace virtdb { namespace connector {

  template <typename MSG, interface::pb::ConnectionType CT>
  struct service_type_map {};

  template <>
  struct service_type_map<interface::pb::MetaDataRequest,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::META_DATA;
  };

  
}}

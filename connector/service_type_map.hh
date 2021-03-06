#pragma once

#include <svc_config.pb.h>
#include <meta_data.pb.h>
#include <data.pb.h>
#include <diag.pb.h>
#include <security.pb.h>
#include <monitoring.pb.h>

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

  template <>
  struct service_type_map<interface::pb::Config,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::CONFIG;
  };

  template <>
  struct service_type_map<interface::pb::Config,
                          interface::pb::ConnectionType::PUB_SUB>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::CONFIG;
  };
  
  template <>
  struct service_type_map<interface::pb::Query,
                          interface::pb::ConnectionType::PUSH_PULL>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::QUERY;
  };

  template <>
  struct service_type_map<interface::pb::Column,
                          interface::pb::ConnectionType::PUB_SUB>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::COLUMN;
  };

  template <>
  struct service_type_map<interface::pb::LogRecord,
                          interface::pb::ConnectionType::PUB_SUB>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::LOG_RECORD;
  };

  template <>
  struct service_type_map<interface::pb::CertStoreRequest,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::CERT_STORE;
  };

  template <>
  struct service_type_map<interface::pb::UserManagerRequest,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::USER_MGR;
  };

  template <>
  struct service_type_map<interface::pb::SourceSystemCredentialRequest,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::SRCSYS_CRED_MGR;
  };
  
  template <>
  struct service_type_map<interface::pb::MonitoringRequest,
                          interface::pb::ConnectionType::REQ_REP>
  {
    static const interface::pb::ServiceType value =
      interface::pb::ServiceType::MONITORING;
  };

  
}}

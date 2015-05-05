#pragma once

#include <gtest/gtest.h>
#include <memory>

namespace virtdb { namespace connector {

  // forward declarations
  class client_context;
  class cert_store_client;
  class srcsys_credential_client;
}}

namespace virtdb { namespace test {
  
  class ConnectorCommon : public ::testing::Test
  {
  protected:
    std::shared_ptr<connector::client_context> cctx_;
    virtual void SetUp();
  };
  
  class ConnEndpointTest : public ConnectorCommon { };
  class ConnConfigTest : public ConnectorCommon { };
  class ConnServerBaseTest : public ConnectorCommon { };

  class ConnCertStoreTest : public ConnectorCommon
  {
  protected:
    bool create_temp_key(connector::cert_store_client & cli,
                         const std::string & name);

    bool approve_temp_key(connector::cert_store_client & cli,
                          const std::string & name);
    
    bool has_temp_key(connector::cert_store_client & cli,
                      const std::string & name);
    
    bool has_approved_key(connector::cert_store_client & cli,
                          const std::string & name);

    bool has_key(connector::cert_store_client & cli,
                 const std::string & name);
    
    bool delete_key(connector::cert_store_client & cli,
                    const std::string & name);
  };
  
  class ConnSrcsysCredTest : public ConnectorCommon
  {
  protected:
    bool set_cred(connector::srcsys_credential_client & cli,
                  const std::string & srcsys,
                  const std::string & token);
    
    bool get_cred(connector::srcsys_credential_client & cli,
                  const std::string & srcsys,
                  const std::string & token);
    
    bool del_cred(connector::srcsys_credential_client & cli,
                  const std::string & srcsys,
                  const std::string & token);
    
    bool set_tmpl(connector::srcsys_credential_client & cli,
                  const std::string & srcsys);
    
    bool get_tmpl(connector::srcsys_credential_client & cli,
                  const std::string & srcsys);
  };
  
  class ConnIpDiscoveryTest : public ConnectorCommon { };
  class ConnLogRecordTest : public ConnectorCommon { };
  
  class ConnQueryTest : public ConnectorCommon { };
  class ConnColumnTest : public ConnectorCommon { };
  class ConnMetaDataTest : public ConnectorCommon { };
  
  class ConnPubSubTest : public ConnectorCommon { };
  class ConnPushPullTest : public ConnectorCommon { };
  class ConnReqRepTest : public ConnectorCommon { };
  
}}


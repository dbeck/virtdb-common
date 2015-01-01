#pragma once

#include <gtest/gtest.h>
#include <connector.hh>

namespace virtdb { namespace test {
  
  class ColumnClientTest : public ::testing::Test { };
  class ColumnServerTest : public ::testing::Test { };

  class ConfigClientTest : public ::testing::Test { };
  class ConfigServerTest : public ::testing::Test { };
  
  class DbConfigClientTest : public ::testing::Test { };
  class DbConfigServerTest : public ::testing::Test { };
  
  class EndpointClientTest : public ::testing::Test { };
  class EndpointServerTest : public ::testing::Test { };
  
  class IpDiscoveryClientTest : public ::testing::Test { };
  class IpDiscoveryServerTest : public ::testing::Test { };
  
  class LogRecordClientTest : public ::testing::Test { };
  class LogRecordServerTest : public ::testing::Test { };
  
  class MetaDataClientTest : public ::testing::Test { };
  class MetaDataServerTest : public ::testing::Test { };

  class SubClientTest : public ::testing::Test { };
  class PubServerTest : public ::testing::Test { };

  class PushClientTest : public ::testing::Test { };
  class PullServerTest : public ::testing::Test { };

  class QueryClientTest : public ::testing::Test { };
  class QueryServerTest : public ::testing::Test { };

  class ReqClientTest : public ::testing::Test { };
  class RepServerTest : public ::testing::Test { };
  
}}


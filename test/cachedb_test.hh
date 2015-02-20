#pragma once

#include <gtest/gtest.h>

namespace virtdb { namespace test {
  
  class XCachedbStoreTest      : public ::testing::Test { };
  class XCachedbDbIdTest       : public ::testing::Test { };
  class XCachedbHashUtilTest   : public ::testing::Test { };
  
  // new classes
  class CachedbColumnDataTest   : public ::testing::Test { };
  class CachedbDBTest           : public ::testing::Test { };
  //query_hasher
  class CachedbQueryHasherTest  : public ::testing::Test { };

}}

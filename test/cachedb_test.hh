#pragma once

#include <gtest/gtest.h>

namespace virtdb { namespace test {
  
  class CachedbHashUtilTest    : public ::testing::Test { };
  
  // new classes
  class CachedbColumnDataTest   : public ::testing::Test { };
  class CachedbDBTest           : public ::testing::Test { };
  //query_hasher
  class CachedbQueryHasherTest  : public ::testing::Test { };

}}

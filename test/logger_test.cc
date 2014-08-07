#include "logger_test.hh"
#include "../logger.hh"

using namespace virtdb::test;
using namespace virtdb::logger;

TEST_F(LoggerTest, LogInfo)
{
  std::string username("me");
  LOG_INFO("test" << V_(username) << "loggged in");
}

TEST_F(LoggerTest, LogError)
{
  LOG_ERROR("error message");
}

TEST_F(LoggerTest, LogTrace)
{
  LOG_TRACE("trace message");
}

TEST_F(LoggerTest, LogScoped)
{
  LOG_SCOPED("scoped message");
}

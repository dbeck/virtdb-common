#pragma once

namespace virtdb { namespace util {
  static const unsigned long DEFAULT_TIMEOUT_MS          = 1000;
  static const unsigned long TINY_TIMEOUT_MS             = 20;
  static const unsigned long SHORT_TIMEOUT_MS            = 100;
  static const unsigned long MAX_SUBSCRIPTION_SIZE       = 1024;
  static const unsigned long DEFAULT_ENDPOINT_EXPIRY_MS  = 3*60*1000; // 3 minutes
  static const unsigned long MAX_0MQ_MESSAGE_SIZE        = 800*1024*1024; // 800 MB is the Max message size
}}

#pragma once

// build time Defines may override these macros
// so the user may switch off them induvidually

#ifndef LOG_INFO_IS_ENABLED
#define LOG_INFO_IS_ENABLED true
#endif

#ifndef LOG_ERROR_IS_ENABLED
#define LOG_ERROR_IS_ENABLED true
#endif

#ifndef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED true
#endif

#ifndef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED true
#endif

// macro helpers for concatenating variable names
#define LOG_INTERNAL_MACRO_CONCAT(A,B) __LOG_INTERNAL__##A##B
#define LOG_INTERNAL_MACRO_CONCAT2(A,B) LOG_INTERNAL_MACRO_CONCAT(A,B)
#define LOG_INTERNAL_LOCAL_VAR(A) LOG_INTERNAL_MACRO_CONCAT2(A,__LINE__)

// these helpers are used to switch between scoped and non-scoped logs
#define LOG_INTERNAL_RECORD_DECL_0
#define LOG_INTERNAL_RECORD_DECL_1 \
  const virtdb::logger::log_record * LOG_INTERNAL_LOCAL_VAR(_log_record_) = 0
#define LOG_INTERNAL_RECORD_SET_0
#define LOG_INTERNAL_RECORD_SET_1 \
  LOG_INTERNAL_LOCAL_VAR(_log_record_) = &_s_log_record_

#define LOG_COMMON_(MSG,LEVEL,ENABLED,SCOPED)                     \
  LOG_INTERNAL_RECORD_DECL_##SCOPED ;                             \
  {                                                               \
    /* the purpose of these static objects is to offload all */   \
    /* possbile log related work from runtime                */   \
                                                                  \
    /* signature is initialized w/ the item count */              \
    static virtdb::logger::signature _s_signature(                \
      virtdb::logger::count_items() <<                            \
      MSG <<                                                      \
      virtdb::logger::end_msg()                                   \
    );                                                            \
                                                                  \
    /* this static variable handles the log header */             \
    static virtdb::logger::log_record _s_log_record_(             \
      __FILE__,                                                   \
      __LINE__,                                                   \
      __func__,                                                   \
      LEVEL,                                                      \
      ENABLED,                                                    \
      /* here we fill the LogPart section of the message */       \
      (                                                           \
        _s_signature << MSG << virtdb::logger::end_msg()          \
      ),                                                          \
      #MSG                                                        \
    );                                                            \
                                                                  \
    /* this part has repeated runtime cost, the upper */          \
    /* parts are amortized accross calls              */          \
    if( _s_log_record_.enabled() )                                \
    {                                                             \
      /* this macro helps scoped logging */                       \
      LOG_INTERNAL_RECORD_SET_##SCOPED ;                          \
      _s_log_record_ << MSG << virtdb::logger::end_msg();         \
    }                                                             \
  }

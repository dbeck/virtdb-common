#pragma once

#include "log_record.hh"
#include "signature.hh"
#include "on_return.hh"
#include <mutex>

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

#ifndef LOG_STATUS_IS_ENABLED
#define LOG_STATUS_IS_ENABLED true
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
  LOG_INTERNAL_LOCAL_VAR(_log_record_) = _s_log_rec_once_

#define LOG_COMMON_(MSG,LEVEL,ENABLED,SCOPED)                       \
  LOG_INTERNAL_RECORD_DECL_##SCOPED ;                               \
  {                                                                 \
    /* once flag gives extra thread safety against multiple */      \
    /* static initialization                                */      \
    static const virtdb::logger::log_record * _s_log_rec_once_ = 0; \
    static std::once_flag _s_once_flag;                             \
    static const char * _s_func_ = __func__;                        \
    std::call_once(_s_once_flag, [&]() {                            \
                                                                    \
      /* the purpose of these static objects is to offload all */   \
      /* possbile log related work from runtime                */   \
      static virtdb::logger::signature _s_signature;                \
                                                                    \
      /* this static variable handles the log header */             \
      static virtdb::logger::log_record _s_log_record_(             \
        __FILE__,                                                   \
        __LINE__,                                                   \
        _s_func_,                                                   \
        LEVEL,                                                      \
        ENABLED,                                                    \
        /* here we fill the LogPart section of the message */       \
        (                                                           \
          _s_signature << MSG << virtdb::logger::end_msg()          \
        ),                                                          \
        _s_signature,                                               \
        #MSG                                                        \
      );                                                            \
      _s_log_rec_once_ = &_s_log_record_;                           \
    });                                                             \
                                                                    \
    /* this part has repeated runtime cost, the upper */            \
    /* parts are amortized accross calls              */            \
    if( _s_log_rec_once_->enabled() )                               \
    {                                                               \
      /* this macro helps scoped logging */                         \
      LOG_INTERNAL_RECORD_SET_##SCOPED ;                            \
      *_s_log_rec_once_ << MSG << virtdb::logger::end_msg();        \
    }                                                               \
  }

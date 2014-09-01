#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <logger.hh>
#include "config_client.hh"
#include "server_base.hh"

namespace virtdb { namespace connector {
  
  template <typename REQ_ITEM,
            typename REP_ITEM>
  class rep_server : public server_base
  {
  public:
  private:
  public:
    
  };
  
}}

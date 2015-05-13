#include <logger/process_info.hh>
#include <logger/symbol_store.hh>
#include <util/relative_time.hh>
#include <util/net.hh>
#include <thread>
#include <memory>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

#ifndef NO_IPV6_SUPPORT
#define VIRTDB_SUPPORTS_IPV6 true
#else
#define VIRTDB_SUPPORTS_IPV6 false
#endif

namespace virtdb { namespace logger {
  
  namespace
  {
    std::string _g_app_name_;
    std::string _g_host_name_;
    std::mutex  _g_name_mutex_;
  }
  
  void
  process_info::set_app_name(const std::string & name)
  {
    std::lock_guard<std::mutex> lock(_g_name_mutex_);
    _g_app_name_ = name;
  }
  
  void
  process_info::set_host_name(const std::string & name)
  {
    std::lock_guard<std::mutex> lock(_g_name_mutex_);
    _g_host_name_ = name;
  }
  
  process_info::process_info() : started_at_(util::relative_time::instance().started_at())
  {
    using namespace std::chrono;
    time_t now = system_clock::to_time_t(system_clock::now());
  
    // TODO: replace with c++11 chrono stuff
    struct tm t({0,0,0, 0,0,0, 0,0,0});
    gmtime_r(&now, &t);
    uint32_t date = static_cast<uint32_t>((t.tm_year+1900)*10000 + (t.tm_mon+1)*100 + t.tm_mday);
    uint32_t time = static_cast<uint32_t>(t.tm_hour*10000 + t.tm_min*100 + t.tm_sec);

    pb_info_.set_startdate(date);
    pb_info_.set_starttime(time);
    pb_info_.set_pid(getpid());
    
    std::srand(std::time(0)+getpid());
    int random_variable = std::rand();
    pb_info_.set_random(random_variable);
    
    {
      std::lock_guard<std::mutex> lock(_g_name_mutex_);
      if( _g_app_name_.empty() )
      {
        const char * app_name = getenv("LOGGER_APP_NAME");
        if( app_name )
        {
          _g_app_name_ = app_name;
        }
      }
      
      if( !_g_app_name_.empty() )
      {
        pb_info_.set_namesymbol(symbol_store::get_symbol_id(_g_app_name_));
      }
    }
    
    {
      std::lock_guard<std::mutex> lock(_g_name_mutex_);
      if( _g_host_name_.empty() )
      {
        const char * host_name = getenv("LOGGER_HOST_NAME");
        if( host_name )
        {
          _g_host_name_ = host_name;
        }
        else
        {
          util::net::string_vector my_ips{util::net::get_own_ips(VIRTDB_SUPPORTS_IPV6)};
          if( my_ips.size() > 0 )
          {
            _g_host_name_ = my_ips[0];
          }
          else
          {
            std::string name{util::net::get_own_hostname()};
            if( !name.empty() )
            {
              _g_host_name_ = name;
            }
          }
        }
      }
      
      if( !_g_host_name_.empty() )
      {
        pb_info_.set_hostsymbol(symbol_store::get_symbol_id(_g_host_name_));
        
      }
    }
  }
  
  process_info &
  process_info::instance()
  {
    static std::unique_ptr<process_info> ptr(new process_info());
    return *ptr;
  }
  
  const interface::pb::ProcessInfo &
  process_info::get_pb() const
  {
    return pb_info_;
  }
  
  const process_info::timepoint &
  process_info::started_at() const
  {
    return started_at_;
  }
}}


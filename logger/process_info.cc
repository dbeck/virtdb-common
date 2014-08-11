#include "process_info.hh"
#include "symbol_store.hh"
#include "../util/relative_time.hh"
#include <thread>
#include <memory>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

namespace virtdb { namespace logger {
  
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

    const char * app_name = getenv("LOGGER_APP_NAME");
    if( app_name )
    {
      pb_info_.set_namesymbol(symbol_store::get_symbol_id(app_name));
    }
    
    const char * host_name = getenv("LOGGER_HOST_NAME");
    if( host_name )
    {
      pb_info_.set_hostsymbol(symbol_store::get_symbol_id(host_name));
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

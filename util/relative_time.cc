#include <util/relative_time.hh>

namespace virtdb { namespace util {

  relative_time::relative_time(const timepoint & start)
  : started_at_(start)
  {
  }
  
  relative_time::relative_time()
  : started_at_(highres_clock::now())
  {
  }
  
  uint64_t
  relative_time::get_msec() const
  {
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    timepoint now(highres_clock::now());
    return duration_cast<milliseconds>(now-started_at_).count();
  }
  
  uint64_t
  relative_time::get_usec() const
  {
    using std::chrono::microseconds;
    using std::chrono::duration_cast;
    timepoint now(highres_clock::now());
    return duration_cast<microseconds>(now-started_at_).count();
  }
  
  const relative_time::timepoint &
  relative_time::started_at() const
  {
    return started_at_;
  }
  
  // create one static instance as well, so this can be
  // used as a global reference timer
  
  const relative_time &
  relative_time::instance()
  {
    static relative_time s_instance;
    return s_instance;
  }
  
}}

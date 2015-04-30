#include "barrier.hh"
#include "constants.hh"
#include <cassert>
#include <chrono>

using namespace virtdb::util;

barrier::barrier(unsigned int nthreads)
: nwaiting_(0),
  nthreads_(nthreads)
{
  assert( nthreads > 0 );
}

barrier::~barrier()
{
  if( nwaiting_ < nthreads_ )
  {
    // we let everyone go because after this object was destroyed
    // noone will be able to continue
    lock l(mutex_);
    nwaiting_ = nthreads_;
    cond_.notify_all();
  }
}

void
barrier::wait()
{
  {
    // register ourself as waiting
    lock l(mutex_);
    ++nwaiting_;
    cond_.notify_all();
  }
  
  while( true )
  {
    lock l(mutex_);
    
    // enough threads are waiting, barrier is done
    if( nwaiting_ >= nthreads_ )
      return;
    
    // doing this in a timed loop, so we don't rely on notifications alone
    cond_.wait_for(l, std::chrono::milliseconds(SHORT_TIMEOUT_MS) );
  }
}

bool
barrier::wait_for(unsigned int timeout_ms)
{
  {
    // register ourself as waiting
    lock l(mutex_);
    ++nwaiting_;
    cond_.notify_all();
  }
  
  using namespace std::chrono;
  typedef std::chrono::steady_clock::time_point time_point_t;
  time_point_t now = steady_clock::now();
  time_point_t max_wait = now + milliseconds{timeout_ms};
  
  while( true )
  {
    lock l(mutex_);
    
    // enough threads are waiting, barrier is done
    // it is late to check timeout condition here, because other threads
    // had already been released
    if( nwaiting_ >= nthreads_ )
      return true;
    
    if( cond_.wait_until(l, max_wait) == std::cv_status::timeout )
    {
      --nwaiting_;
      return false;
    }
  }
}


bool
barrier::ready()
{
  lock l(mutex_);
  return (nwaiting_ >= nthreads_);
}

void
barrier::reset()
{
  lock l(mutex_);
  nwaiting_ = 0;
}


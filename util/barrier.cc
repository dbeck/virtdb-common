#include "barrier.hh"
#include <cassert>

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
    
    cond_.wait(l,[this]{ return (nwaiting_ >= nthreads_); });
  }
}

#pragma once

#include <memory>
#include <cassert>

namespace virtdb { namespace util {

class mempool 
{
protected:
  // the chosen integral type is going to be the smallest unit
  // the allocated memory is aligned to that unit
  typedef long long item;

  // these are expected to be reimplemented in child classes
  virtual item * allocate_items(size_t n)
  {
    return new item[n];
  }

  virtual void delete_items(item * p)
  {
    delete [] p;
  }

  virtual mempool * allocate_pool(size_t byte_size, size_t next_size)
  {
    return new mempool(byte_size,next_size);
  }

  virtual void delete_pool(mempool * p)
  {
    delete p;
  }

private:
  enum { item_size_ = sizeof(item) };

  size_t    allocated_items_;
  size_t    free_items_;
  size_t    next_size_;
  item *    pool_;
  mempool * next_;
  mempool * last_;

  mempool * last()
  {
    mempool * ret = last_;
    mempool * p = ret->next_;
    while( p != nullptr ) { ret = p; p = ret->next_; }
    last_ = ret;
    return ret;
  }

  item * next_item()
  {
    size_t used = (allocated_items_-free_items_);
    return pool_+used;
  }

  mempool() = delete;
  mempool(const mempool &) = delete;
  mempool& operator=(const mempool &) = delete;

  static constexpr size_t aligned_size(size_t sz)
  {
    return (sz+item_size_-1)/item_size_;
  }

public:
  typedef std::shared_ptr<mempool> sptr;

  mempool(size_t byte_size, size_t next_size=0)
  : allocated_items_(aligned_size(byte_size)),
    free_items_(allocated_items_),
    next_size_(next_size?aligned_size(next_size):allocated_items_),
    pool_(allocate_items(allocated_items_)),
    next_(nullptr),
    last_(this) {}

  virtual ~mempool() 
  {
    clear();
  }
  
  void clear()
  {
    if( next_ )
    {
      delete_pool(next_);
      next_ = nullptr;
    }
    if( pool_ )
    {
      delete_items(pool_);
      pool_ = nullptr;
    }
  }
   
  const size_t allocated_bytes() const
  {
    size_t ret = allocated_items_*item_size_;
    mempool * p = next_;
    while( p )
    {
      ret += (p->allocated_items_*item_size_);
      p = p->next_;
    }
    return ret;
  }
  
  const size_t next_size() const
  {
    return next_size_;
  }

  static constexpr size_t item_size() { return item_size_; }

  template <typename T>
  static constexpr size_t item_count(size_t n)
  {
    return ((n*sizeof(T)) /* bytes */
           +item_size_-1) /* padded size */
           / item_size_;  /* size in items */
  }

  template <typename T>
  T * allocate(size_t n)
  {
    assert( n != 0 );
    if( !n ) return nullptr;
    size_t count = item_count<T>(n);

    mempool * p = last();
    assert( p != nullptr );
    
    if( count > p->free_items_ )
    {
      // needs a bigger pool
      size_t to_be_allocated = next_size_; 
      while( count > to_be_allocated )
      {
        to_be_allocated += next_size_;
      }
      p->next_ = allocate_pool(to_be_allocated*item_size_,next_size_); 
      p = p->next_;
    }

    item * reti = p->next_item();
    *reti = 0;
    T * ret = reinterpret_cast<T*>(reti);
    assert( p->free_items_ >= count );
    p->free_items_ -= count;
    return ret;
  }

  template <typename T>
  void reuse(size_t n)
  {
    size_t count = (n*sizeof(T))/item_size_;
    if( !count ) return;
    mempool * p = last();
    assert( p != nullptr );
    assert( (count+p->free_items_) <= p->allocated_items_ );
    p->free_items_ += count;
  }
};

}}


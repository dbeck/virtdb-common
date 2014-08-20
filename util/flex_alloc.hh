#pragma once

#include <memory>

namespace virtdb { namespace util {
  
  template <typename T, unsigned long STATIC_SIZE>
  class flex_alloc final
  {
    T                       static_[STATIC_SIZE];
    std::unique_ptr<T []>   dynamic_;
    T                     * ptr_;
    unsigned long           size_;
    
    void dyn_alloc(unsigned int size)
    {
      dynamic_.reset(new T[size]);
      ptr_ = dynamic_.get();
      size_ = size;
    }
    
  public:
    flex_alloc(unsigned long size)
    : ptr_(static_),
      size_(STATIC_SIZE)
    {
      if( size > STATIC_SIZE )
        dyn_alloc(size);
    }

    flex_alloc(int size)
    : ptr_(static_),
      size_(STATIC_SIZE)
    {
      if( size > (int)STATIC_SIZE )
        dyn_alloc(size);
    }
    
    T * get() { return ptr_; }
    
    unsigned long size() const { return size_; }
    
    ~flex_alloc() {}
    
  private:
    flex_alloc() = delete;
    flex_alloc(const flex_alloc &) = delete;
    flex_alloc & operator=(const flex_alloc &) = delete;
  };
}}

#pragma once

#include <gtest/gtest.h>
#include "util/value_type.hh"
#include <memory>

namespace virtdb { namespace test {

  class ValueTypeTest : public ::testing::Test
  {
  protected:
    typedef std::shared_ptr<virtdb::interface::pb::ValueType> value_type_sptr;
    value_type_sptr value_type_sptr_;
    
    ValueTypeTest() : value_type_sptr_(new virtdb::interface::pb::ValueType) {}

    virtual void SetUp() { }
    virtual void TearDown() {}

    std::string
    toString()
    {
      return value_type_sptr_->DebugString();
    }
    
    template <typename T>
    int size()
    {
      return virtdb::interface::value_type<T>::size(*value_type_sptr_);
    }
    
    bool is_null(int index)
    {
      return virtdb::interface::value_type_base::is_null(*value_type_sptr_,
                                                         index);
    }

    template <typename T>
    bool is_null(int index)
    {
      return virtdb::interface::value_type<T>::is_null(*value_type_sptr_,
                                                       index);
    }
    
    void set_null(int index,
                  bool val=true)
    {
      virtdb::interface::value_type_base::set_null(*value_type_sptr_,
                                                   index,
                                                   val);
    }

    template <typename T>
    void set_null(int index,
                  bool val=true)
    {
      virtdb::interface::value_type<T>::set_null(*value_type_sptr_,
                                                 index,
                                                 val);
    }

    
    template <typename VAL_ITER>
    void set(VAL_ITER val_begin,
             VAL_ITER val_end)
    {
      typedef typename VAL_ITER::value_type val_t;
      virtdb::interface::value_type<val_t>::set(*value_type_sptr_,
                                                val_begin,
                                                val_end);
    }
    
    virtdb::interface::pb::Kind
    get_kind()
    {
      return value_type_sptr_->type();
    }
    
    template <typename T>
    T get(int index,
          const T & default_value)
    {
      return virtdb::interface::value_type<T>::get(*value_type_sptr_,
                                                   index,
                                                   default_value);
    }
  };

} } // virtdb::test


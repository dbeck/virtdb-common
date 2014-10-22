#pragma once

#include "common.pb.h"
#include <string>

// TODO : polymorphic getter

namespace virtdb { namespace util {

  struct value_type_base
  {
    static bool
    is_null(interface::pb::ValueType & pb_vt,
            int index)
    {
      bool ret = false;
      if( pb_vt.isnull_size() > index )
        ret = pb_vt.isnull(index);
      return ret;
    }

    static void
    set_null(interface::pb::ValueType & pb_vt,
             int index,
             bool val=true)
    {
      // fill the null array up to the
      for( int i=pb_vt.isnull_size(); i<=index; ++i )
        pb_vt.add_isnull(false);
      pb_vt.set_isnull(index, val);
    }
  };

  template <typename T> struct value_type {};

  template <>
  struct value_type<std::string> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::STRING;
    typedef std::string stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.stringvalue_size() )
        pb_vt.clear_stringvalue();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_stringvalue(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        const stored_type & v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.stringvalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.stringvalue_size() <= index )
        return default_value;
      else
        return pb_vt.stringvalue(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.stringvalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_stringvalue()->Get(index);
    }
  };

  template <>
  struct value_type<const char *> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::STRING;
    typedef std::string stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.stringvalue_size() )
        pb_vt.clear_stringvalue();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_stringvalue(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        const char * v,
        interface::pb::Kind val_kind=kind)
    {
      const char ** val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.stringvalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.stringvalue_size() <= index )
        return default_value;
      else
        return pb_vt.stringvalue(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.stringvalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_stringvalue()->Get(index);
    }
  };

  template <>
  struct value_type<int32_t> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::INT32;
    typedef int32_t stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.int32value_size() )
        pb_vt.clear_int32value();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_int32value(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.int32value_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.int32value_size() <= index )
        return default_value;
      else
        return pb_vt.int32value(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.int32value_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_int32value()->Get(index);
    }
  };

  template <>
  struct value_type<int64_t> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::INT64;
    typedef int64_t stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.int64value_size() )
        pb_vt.clear_int64value();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_int64value(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.int64value_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.int64value_size() <= index )
        return default_value;
      else
        return pb_vt.int64value(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.int64value_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_int64value()->Get(index);
    }
  };

  template <>
  struct value_type<uint32_t> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::UINT32;
    typedef uint32_t stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.uint32value_size() )
        pb_vt.clear_uint32value();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_uint32value(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.uint32value_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.uint32value_size() <= index )
        return default_value;
      else
        return pb_vt.uint32value(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.uint32value_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_uint32value()->Get(index);
    }
  };

  template <>
  struct value_type<uint64_t> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::UINT64;
    typedef uint64_t stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.uint64value_size() )
        pb_vt.clear_uint64value();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_uint64value(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.uint64value_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.uint64value_size() <= index )
        return default_value;
      else
        return pb_vt.uint64value(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.uint64value_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_uint64value()->Get(index);
    }
  };

#ifdef COMMON_MAC_BUILD
#ifndef COMMON_LINUX_BUILD
  template <>
  struct value_type<unsigned long> : public value_type<uint64_t>
  {
  };
#endif
#endif

  template <>
  struct value_type<double> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::DOUBLE;
    typedef double stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.doublevalue_size() )
        pb_vt.doublevalue();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_doublevalue(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.doublevalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.doublevalue_size() <= index )
        return default_value;
      else
        return pb_vt.doublevalue(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.doublevalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_doublevalue()->Get(index);
    }
  };

  template <>
  struct value_type<float> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::FLOAT;
    typedef float stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.floatvalue_size() )
        pb_vt.floatvalue();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_floatvalue(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        stored_type v,
        interface::pb::Kind val_kind=kind)
    {
      const stored_type * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.floatvalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.floatvalue_size() <= index )
        return default_value;
      else
        return pb_vt.floatvalue(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.floatvalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_floatvalue()->Get(index);
    }
  };

  template <>
  struct value_type<bool> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::BOOL;
    typedef bool stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.boolvalue_size() )
        pb_vt.boolvalue();
      for( auto it=begin ; it != end ; ++it )
        pb_vt.add_boolvalue(*it);
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        bool v,
        interface::pb::Kind val_kind=kind)
    {
      const bool * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.boolvalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.boolvalue_size() <= index )
        return default_value;
      else
        return pb_vt.boolvalue(index);
    }

    static const stored_type&
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.boolvalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
        return pb_vt.mutable_boolvalue()->Get(index);
    }
  };

  struct byte_data
  {
      char *   data;
      size_t   len;
  };

  template <>
  struct value_type<byte_data> : public value_type_base
  {
    static const interface::pb::Kind kind = interface::pb::Kind::BYTES;
    typedef bool stored_type;

    template <typename ITER>
    static void
    set(interface::pb::ValueType & pb_vt,
        ITER begin,
        ITER end,
        interface::pb::Kind val_kind=kind)
    {
      pb_vt.set_type(val_kind);
      if( pb_vt.bytesvalue_size() )
        pb_vt.bytesvalue();
      for( auto it=begin ; it != end ; ++it )
      {
        pb_vt.add_bytesvalue(std::string(it->data, it->len));
      }
    }

    static void
    set(interface::pb::ValueType & pb_vt,
        byte_data v,
        interface::pb::Kind val_kind=kind)
    {
      const byte_data * val_ptr = &v;
      set(pb_vt, val_ptr, val_ptr+1, val_kind);
    }

    static int
    size(interface::pb::ValueType & pb_vt)
    {
      return pb_vt.bytesvalue_size();
    }

    static stored_type
    get(const interface::pb::ValueType & pb_vt,
        int index,
        const stored_type & default_value)
    {
      if( pb_vt.bytesvalue_size() <= index )
        return default_value;
      else
      {
        std::string bytes_as_string = pb_vt.bytesvalue(index);
        return (bytes_as_string.data(), bytes_as_string.size());
      }
    }

    static stored_type
    get(interface::pb::ValueType & pb_vt,
        int index)
    {
      if( pb_vt.bytesvalue_size() <= index )
        throw std::out_of_range(std::to_string(index) + " is out of range");
      else
      {
        const std::string& bytes_as_string = pb_vt.mutable_bytesvalue()->Get(index);
        return (bytes_as_string.data(), bytes_as_string.size());
      }
    }
  };

}} // virtdb::interface

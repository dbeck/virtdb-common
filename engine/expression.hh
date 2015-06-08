#pragma once

// protocol buffer
#include "data.pb.h"

// standard headers
#include <memory>

namespace virtdb {  namespace engine {

  class expression {
  public:
    typedef std::shared_ptr<expression> sptr;
    
  private:
    virtdb::interface::pb::SimpleExpression simple;
    int          column_id;
    sptr         left_ptr;
    sptr         right_ptr;
    std::string  operand_value;
    
    inline bool is_composite() const
    {
      return left_ptr != nullptr && right_ptr != nullptr;
    }
    
    void fill(virtdb::interface::pb::Expression& proto) const;
    
  public:
    expression() : column_id{-1} { }
    virtual ~expression() { }
    
    // Columns - for adding columns to query
    std::map<int, std::string> columns() const;
    
    // expression
    void set_operand(std::string value);
    const ::std::string& operand() const;
    
    // SimpleExpression
    void set_variable(int id, std::string value);
    const ::std::string& variable() const;
    void set_value(std::string value);
    const ::std::string& value() const;
    
    // CompositeExpression
    bool set_composite(sptr left, sptr right);
    sptr left () const { return left_ptr; }
    sptr right () const { return right_ptr; }
    
    // Accessing encapsulated object
    virtdb::interface::pb::Expression get_message() const;
  };

}}

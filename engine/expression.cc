#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "expression.hh"

using namespace virtdb::interface;

namespace virtdb {  namespace engine {

std::map<int, std::string> expression::columns() const
{
    std::map<int, std::string> ret;
    if (!is_composite())
    {
        ret[column_id] = variable();
    }
    else
    {
        std::map<int, std::string> left_columns = left_ptr->columns();
        for (auto it = left_columns.begin(); it != left_columns.end(); it++)
        {
            ret[it->first] = it->second;
        }
        std::map<int, std::string> right_columns = right_ptr->columns();
        for (auto it = right_columns.begin(); it != right_columns.end(); it++)
        {
            ret[it->first] = it->second;
        }
    }
    return ret;
}

void expression::set_operand(std::string value) {
    operand_value = value;
}

const ::std::string& expression::operand() const {
    return operand_value;
}

void expression::set_variable(int id, std::string value)
{
    column_id = id;
    simple.set_variable(value);
}

const ::std::string& expression::variable() const {
    return simple.variable();
}

void expression::set_value(std::string value) {
    simple.set_value(value);
}

const ::std::string& expression::value() const {
    return simple.value();
}

bool expression::set_composite(sptr _left, sptr _right)
{
    if (!_left || !_right || _left == _right)
    {
        return false;
    }
    left_ptr   = _left;
    right_ptr  = _right;
    return true;
}

void expression::fill(pb::Expression& proto) const
{
    proto.set_operand(operand());
    if (!is_composite())
    {
        proto.mutable_simple()->set_variable(variable());
        proto.mutable_simple()->set_value(value());
    }
    else
    {
        pb::Expression* left_proto = proto.mutable_composite()->mutable_left();
        left()->fill(*left_proto);
        pb::Expression* right_proto = proto.mutable_composite()->mutable_right();
        right()->fill(*right_proto);
    }
}

pb::Expression expression::get_message() const
{
    pb::Expression ret;
    this->fill(ret);
    return ret;
}

}}

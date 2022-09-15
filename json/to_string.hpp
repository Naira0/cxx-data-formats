
#include "type.hpp"

namespace JSON
{
    std::string to_string(const array_t &array);
    std::string to_string(const object_t &object, int nest_level = 1);
    std::string to_string(const Value &value, int nest_level = 1);
}

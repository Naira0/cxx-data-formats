//
// Created by arian on 9/15/22.
//

#include "to_string.hpp"

std::string trimmed_itoa(double value)
{
    std::string str = std::to_string(value);

    size_t pos = str.find('.');

    // this is not efficient but relative to the average string size it does the job ok
    if ((int) value == value)
    {
        str = str.substr(0, pos);
    } else
    {
        bool found = false;

        for (size_t i = str.size() - 1; i > pos; i--)
        {
            if (str[i] != '0')
            {
                found = true;
                pos = i + 1;
                break;
            }
        }

        if (found)
            str = str.substr(0, pos);
    }

    return str;
}

namespace JSON
{
    std::string to_string(const Value &value, int nest_level)
    {
        switch (value.index())
        {
            case String:
                return "\"" + std::get<String>(value) + "\"";
            case Number:
                return trimmed_itoa(std::get<Number>(value));
            case Bool:
                return (std::get<Bool>(value) ? "true" : "false");
            case Null:
                return "null";
            case Object:
                return to_string(std::get<Object>(value), ++nest_level);
            case Array:
                return to_string(std::get<Array>(value));
        }
    }

    std::string to_string(const array_t &array)
    {
        if (array.empty())
            return "[]";

        std::string output = "[ " + to_string(array[0]);

        for (size_t i = 1; i < array.size(); i++)
            output += ", " + to_string(array[i]);

        output += " ]";

        return output;
    }

    std::string to_string(const object_t &object, int nest_level)
    {
        if (object.empty())
            return "{}";

        std::string output = "{\n";

#define FILL for (int i = 0; i < nest_level; i++) output += '\t'

        size_t j = 0;

        for (auto &[key, value] : object)
        {
            FILL;

            output += "\"" + key + "\": " + to_string(value, nest_level) + ",\n";
        }

        // removes trailing comma
        output[output.size()-2] = output[output.size()-1];
        output.pop_back();

        nest_level--;

        FILL;

        output += '}';

        return output;

#undef FILL
    }
}

#pragma once

#include <variant>
#include <cstdint>
#include <string>
#include <map>
#include <vector>

#include "../map.hpp"

namespace fmt
{
    template<typename T>
    inline std::string string_of(const T &value);
}

namespace JSON
{
    // json value type to act as an abstraction for std::get<T>()
    enum Type : uint8_t
    {
        String, Number, Bool, Null, Object, Array
    };

    struct Value;

    //  NOTE does not preserve insertion order
    using object_t = dtf::Map<std::string, Value>;
    using array_t = std::vector<Value>;
    using record_t = std::pair<std::string, Value>;

    using value_t = std::variant<
            std::string,
            double,
            bool,
            std::nullptr_t,
            object_t,
            array_t>;

    struct Value : public value_t
    {
        Value() = default;

        template<typename T>
        Value(T value)
        {
            set_value(std::forward<T>(value));
        }

//        Value &operator=(Value &&value) noexcept
//        {
//
//            return *this;
//        }
//
//        Value &operator=(const Value &value) noexcept
//        {
//            return *this;
//        }

        Value &operator=(object_t &&value)
        {
            set_value(std::forward<object_t>(value));
            return *this;
        }

        Value &operator=(std::initializer_list<Value> &&value)
        {
            emplace<array_t>(value);
            return *this;
        }

        template<typename T>
        Value &operator=(T value)
        {
            set_value(std::forward<T>(value));
            return *this;
        }

        std::string to_string() const
        {
            using namespace JSON;

#define GET(T) case T: return fmt::string_of(std::get<T>(*this))

            switch (index())
            {
                GET(String);
                GET(Number);
                GET(Bool);
                GET(Null);
                GET(Object);
                GET(Array);
            }

#undef GET
        }


    private:
        template<typename T>
        void set_value(T &&value)
        {
            if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
                emplace<double>((double) value);
            else if constexpr (std::is_constructible_v<std::string, T> && !std::is_same_v<T, std::nullptr_t>)
                emplace<std::string>(value);
            else
                emplace<T>(value);
        }
    };
}
#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <filesystem>
#include <cctype>

namespace fmt
{
    template<typename T>
    inline std::string string_of(const T& value);

    template<typename... A>
    inline int print(std::string_view fmt, A&&... a);
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
    using object_t = std::map<std::string, Value>;
    using array_t  = std::vector<Value>;
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

        Value& operator=(object_t &&value)
        {
            set_value(std::forward<object_t>(value));
            return *this;
        }

        Value& operator=(std::initializer_list<Value> &&value)
        {
            emplace<array_t>(value);
            return *this;
        }

        template<typename T>
        Value& operator=(T value)
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
            if constexpr(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
                emplace<double>((double)value);
            else if constexpr(std::is_constructible_v<std::string, T> && !std::is_same_v<T, std::nullptr_t>)
                emplace<std::string>(value);
            else
                emplace<T>(value);
        }
    };

    class Parser
    {
    public:

        Parser(std::string_view source) :
                m_source(source)
        {}

        std::optional<object_t> parse()
        {
            skip_chars();

            if (!match('{'))
            {
                m_error = "did not find root object";
                return std::nullopt;
            }

            object_t object = parse_object();

            if (has_error())
                return std::nullopt;

            return object;
        }

        std::string_view error() const
        {
            return m_error;
        }

        bool has_error() const
        {
            return !m_error.empty();
        }

    private:
        size_t m_current{}, m_offset{};
        std::string_view m_source;
        std::string_view m_error;

        object_t parse_object()
        {
            object_t object;

            skip_chars();

            while ((!at_end() && peek() != '}') && !has_error())
            {
                m_current = m_offset;

                char c = advance();

                switch (c)
                {
                    case '}': return object;
                    case '"':
                    {
                        auto [key, value] = parse_record();

                        if (has_error())
                            return {};

                        object.emplace(key, value);

                        if (!validate_end())
                        {
                            m_error = "invalid character found";
                            return {};
                        }

                        break;
                    }
                    default:
                    {
                        m_error = "unexpected character found";
                    }
                }
            }

            if (at_end() && peek() != '}')
                m_error = "unterminated object found";

            return object;
        }

        bool validate_end()
        {
            skip_chars();

            char next = peek();

            switch (next)
            {
                case ',':
                {
                    m_offset++;

                    skip_chars();

                    char c = peek();

                    return c == '"';
                }
                case '}': return true;
                default: return false;
            }
        }

        void skip_chars()
        {
            while (!at_end())
            {
                switch (peek())
                {
                    case ' ':
                    case '\n':
                    case '\t':
                    case '\r': m_offset++; break;
                    default:
                        return;
                }
            }
        }

        char escape_string(char c)
        {
            switch (c)
            {
                case '"':  return '"';
                case '\\': return '\\';
                case '/':  return '/';
                case 'b':  return '\b';
                case 'f':  return '\f';
                case 'n':  return '\n';
                case 'r':  return '\r';
                case 't':  return '\t';
                default: return '\0';
            }
        }

        std::string parse_string(bool allow_escaping)
        {
            m_current++;

            std::string output;

            while (!at_end() && peek() != '"')
            {
                char c = advance();

                if (c == '\\' && allow_escaping)
                {
                    char escaped = escape_string(advance());
                    if (escaped == '\0')
                    {
                        m_error = "illegal escape character found";
                        return {};
                    }
                    output += escaped;
                }
                else
                    output += c;
            }

            if (peek() != '"')
            {
                m_error = "unterminated string found";
                return {};
            }

            m_offset++;

            return output;
        }

        double parse_number()
        {
        scan:
            while (!at_end() && std::isdigit(peek()))
                m_offset++;

            if (match('.'))
            {
                goto scan;
            }

            return std::stod(slice());
        }

        inline bool cmp(std::string_view str)
        {
            for (char c : str)
            {
                if (!match(c))
                    return false;
            }

            return true;
        }

        inline bool parse_bool()
        {
            char c = peek();

            if (c == 'r')
                return cmp("rue");
            else if (c == 'a')
                return !cmp("alse");
            else
            {
                m_error = "invalid keyword found";
                return false;
            }
        }

        Value parse_value()
        {
            skip_chars();

            m_current = m_offset;

            char c = advance();

            switch (c)
            {
                case '"': return parse_string(true);
                case '[': return parse_array();
                case '{': return parse_object();
                default:
                {
                    if (std::isdigit(c))
                        return parse_number();
                    else if (c == 't' || c == 'f')
                        return parse_bool();
                    else if (c == 'n')
                    {
                        if (cmp("ull"))
                            return { nullptr };
                        else
                            goto error;
                    }
                    else
                    {
                        error:
                            m_error = "invalid keyword found";
                            return {};
                    }
                }
            }
        }

        record_t parse_record()
        {
            std::string key = parse_string(false);

            skip_chars();

            if(!match(':'))
            {
                m_error = "unexpected character found";
                return {};
            }

            return {key, parse_value()};
        }

        array_t parse_array()
        {
            skip_chars();

            if (match(']'))
                return {};

            array_t array;

            do
            {
                Value value = parse_value();

                array.emplace_back(value);

                skip_chars();
            } while(match(',') && peek() != ']');

            if (peek() != ']')
                m_error = "unterminated array found";

            m_offset++;

            return array;
        }

        inline bool at_end() const
        {
            return m_offset >= m_source.size();
        }

        inline char peek() const
        {
            if (at_end())
                return '\0';
            return m_source[m_offset];
        }

        inline bool match(char c)
        {
            if (peek() == c)
            {
                m_offset++;
                return true;
            }
            return false;
        }

        inline char advance()
        {
            if (at_end())
                return '\0';
            return m_source[m_offset++];
        }

        inline std::string slice()
        {
            return std::string{ m_source.substr(m_current, m_offset - m_current) };
        }
    };
}

#include "fmt.hpp"
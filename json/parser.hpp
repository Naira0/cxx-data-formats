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

#include "type.hpp"

namespace fmt
{
    template<typename... A>
    inline int print(std::string_view fmt, A&&... a);
}

namespace JSON
{
    class Parser
    {
    public:

        Parser(std::string_view source) :
                m_source(source)
        {}

        std::optional<object_t> parse();

        std::string_view error() const
        {
            return m_error;
        }

        bool has_error() const
        {
            return !m_error.empty();
        }

    private:
        size_t
            m_current{},
            m_offset{};
        std::string_view
            m_source,
            m_error;

        object_t parse_object();

        bool validate_end();

        void skip_chars();

        char escape_string(char c) const;

        std::string parse_string(bool allow_escaping);

        double parse_number();

        inline bool cmp(std::string_view str);

        inline bool parse_bool();

        Value parse_value();

        record_t parse_record();

        array_t parse_array();

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


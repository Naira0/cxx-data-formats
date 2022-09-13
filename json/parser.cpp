#include "parser.hpp"

std::optional<JSON::object_t> JSON::Parser::parse()
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

JSON::object_t JSON::Parser::parse_object()
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

bool JSON::Parser::validate_end()
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

void JSON::Parser::skip_chars()
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

inline char JSON::Parser::escape_string(char c) const
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

std::string JSON::Parser::parse_string(bool allow_escaping)
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

double JSON::Parser::parse_number()
{
    scan:
    while (!at_end() && std::isdigit(peek()))
        m_offset++;

    if (match('.'))
        goto scan;

    return std::stod(slice());
}

bool JSON::Parser::cmp(std::string_view str)
{
    for (char c : str)
    {
        if (!match(c))
            return false;
    }

    return true;
}

bool JSON::Parser::parse_bool()
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

JSON::Value JSON::Parser::parse_value()
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

JSON::record_t JSON::Parser::parse_record()
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

JSON::array_t JSON::Parser::parse_array()
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
#include "util/WpaText.hpp"
#include <cctype>

std::string WpaText::decode(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] != '\\' || i + 1 == text.size())
        {
            out += text[i];
            continue;
        }
        switch (const char c = text[i + 1])
        {
        case '\\': case '"': out += c; ++i; break;
        case 'e': out += '\x1b'; ++i; break;
        case 'n': out += '\n'; ++i; break;
        case 'r': out += '\r'; ++i; break;
        case 't': out += '\t'; ++i; break;
        case 'x':
            if (i + 3 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 2]))
                && std::isxdigit(static_cast<unsigned char>(text[i + 3])))
            {
                out += static_cast<char>(std::stoi(text.substr(i + 2, 2), nullptr, 16));
                i += 3;
                break;
            }
            out += text[i];
            break;
        default: out += text[i]; break;
        }
    }
    return out;
}

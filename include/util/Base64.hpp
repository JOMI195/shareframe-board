#pragma once
#include <string>
#include <vector>

#include "util/NonInstantiable.hpp"

class Base64 : NonInstantiable
{
public:
    static std::vector<unsigned char> decode(const std::string& input);
    static std::string encode(const unsigned char* data, size_t len);

    /// URL-safe base64 decode: converts -→+ and _→/ before standard decode.
    static std::vector<unsigned char> urlDecode(const std::string& input);
};

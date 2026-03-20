#pragma once
#include <cstdint>
#include <string>

struct Token
{
    std::string value;
    int64_t     expiresAt; // Unix timestamp (seconds since epoch)
};

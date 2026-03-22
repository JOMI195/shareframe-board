#pragma once
#include <filesystem>
#include <string>

struct Image
{
    int64_t id;
    std::string sender;
    std::filesystem::path imagePath;
    int64_t expiresAt; // Unix timestamp (seconds since epoch)
};

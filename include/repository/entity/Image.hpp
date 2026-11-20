#pragma once
#include <filesystem>
#include <string>

struct Image
{
    static constexpr int64_t kNeverExpires = 0;

    int64_t id;
    std::string sender;
    std::filesystem::path imagePath;
    int64_t expiresAt; // Unix timestamp (seconds since epoch), or kNeverExpires
    int64_t receivedAt = 0; // Unix timestamp; 0 for pictures stored before it was recorded
};

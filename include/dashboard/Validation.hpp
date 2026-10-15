#pragma once
#include "settings/ImageQuality.hpp"
#include "settings/PlaybackOrder.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <algorithm>
#include <string>
#include <vector>

namespace dashboard::Validation {

// WiFi
constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MAX_PASSWORD_LENGTH = 63;
constexpr size_t MIN_PASSWORD_LENGTH = 8;
constexpr size_t MAX_NETWORK_NAME_LENGTH = 128;

// Slideshow
constexpr int MIN_INTERVAL_SECS = 180;
constexpr int MAX_INTERVAL_SECS = 86400;

// Slideshow images
constexpr size_t MAX_CUSTOM_ORDER_LENGTH = 1000;

// Logs
constexpr int MAX_LOG_LINES = 10000;
constexpr int DEFAULT_LOG_LINES = 100;

inline bool containsControlChars(const std::string& s)
{
    return std::ranges::any_of(s, [](unsigned char c) { return c < 0x20 && c != ' '; });
}

inline bool isValidSsid(const std::string& ssid)
{
    return !ssid.empty() && ssid.size() <= MAX_SSID_LENGTH && !containsControlChars(ssid);
}

inline bool isValidPassword(const std::string& password)
{
    return password.size() >= MIN_PASSWORD_LENGTH
        && password.size() <= MAX_PASSWORD_LENGTH
        && !containsControlChars(password);
}

// hostapd only accepts 8-63 printable-ASCII passphrases; anything else makes
// it exit on startup, which would brick the AP fallback.
inline bool isValidWpaPassphrase(const std::string& password)
{
    return password.size() >= MIN_PASSWORD_LENGTH
        && password.size() <= MAX_PASSWORD_LENGTH
        && std::ranges::all_of(password, [](unsigned char c) { return c >= 0x20 && c <= 0x7e; });
}

inline bool isValidNetworkName(const std::string& name)
{
    return !name.empty() && name.size() <= MAX_NETWORK_NAME_LENGTH && !containsControlChars(name);
}

inline bool isAllowedServiceName(const std::string& name,
                                 const std::vector<std::string>& allowed)
{
    return std::ranges::find(allowed, name) != allowed.end();
}

inline int clampLogLines(int lines)
{
    if (lines <= 0) return DEFAULT_LOG_LINES;
    return std::min(lines, MAX_LOG_LINES);
}

inline bool isValidSlideshowAction(const std::string& action)
{
    return action == "start" || action == "stop";
}

inline bool isValidIntervalSecs(const int secs)
{
    return secs >= MIN_INTERVAL_SECS && secs <= MAX_INTERVAL_SECS;
}

inline bool isValidHour(const int hour)
{
    return hour >= 0 && hour <= 23;
}

inline bool isValidImageQuality(const std::string& quality)
{
    return imageQualityFromString(quality).has_value();
}

inline bool isValidPlaybackMode(const std::string& mode)
{
    return playbackModeFromString(mode).has_value();
}

/// Ids of a custom play order, duplicates dropped; nullopt unless `j` is an
/// array of at most MAX_CUSTOM_ORDER_LENGTH integers.
inline std::optional<std::vector<int64_t>> parseCustomOrder(const nlohmann::json& j)
{
    if (!j.is_array() || j.size() > MAX_CUSTOM_ORDER_LENGTH)
        return std::nullopt;
    std::vector<int64_t> ids;
    for (const auto& v : j)
    {
        if (!v.is_number_integer())
            return std::nullopt;
        if (const auto id = v.get<int64_t>(); std::ranges::find(ids, id) == ids.end())
            ids.push_back(id);
    }
    return ids;
}

} // namespace dashboard::Validation

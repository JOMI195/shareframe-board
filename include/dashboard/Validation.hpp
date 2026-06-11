#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace dashboard::Validation {

// WiFi
constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MAX_PASSWORD_LENGTH = 63;
constexpr size_t MIN_PASSWORD_LENGTH = 8;
constexpr size_t MAX_NETWORK_NAME_LENGTH = 128;

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

} // namespace dashboard::Validation

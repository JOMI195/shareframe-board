#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace logging
{

inline constexpr std::size_t kMaxLogPayloadBytes = 1024;

inline std::string summarizePayloadForLog(
    std::string_view payload,
    std::string_view label = "payload",
    std::size_t maxBytes = kMaxLogPayloadBytes)
{
    if (payload.size() <= maxBytes)
        return std::string(payload);

    std::string summary = "<";
    if (!label.empty())
    {
        summary += label;
        summary += " ";
    }
    summary += "truncated: ";
    summary += std::to_string(payload.size());
    summary += " bytes>";
    return summary;
}

} // namespace logging

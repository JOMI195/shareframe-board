#pragma once
#include <ctime>
#include <optional>
#include <string>

/// What the panel does inside the quiet-hours window.
enum class NightStyle
{
    Slower,
    Hold,
    Pause
};

inline std::string toString(const NightStyle style)
{
    switch (style)
    {
    case NightStyle::Slower: return "slower";
    case NightStyle::Hold:   return "hold";
    case NightStyle::Pause:  return "pause";
    }
    return "hold";
}

inline std::optional<NightStyle> nightStyleFromString(const std::string& s)
{
    if (s == "slower") return NightStyle::Slower;
    if (s == "hold")   return NightStyle::Hold;
    if (s == "pause")  return NightStyle::Pause;
    return std::nullopt;
}

/// What the quiet hours ask of the panel right now.
enum class NightState
{
    Day,
    Slower,
    Hold,
    Pause
};

namespace night
{
/// Night only while the quiet hours are switched on and their window is open.
constexpr NightState state(const bool enabled, const NightStyle style, const int secsUntilEnd)
{
    if (!enabled || secsUntilEnd <= 0)
        return NightState::Day;
    switch (style)
    {
    case NightStyle::Slower: return NightState::Slower;
    case NightStyle::Hold: return NightState::Hold;
    case NightStyle::Pause: return NightState::Pause;
    }
    return NightState::Day;
}

/// Seconds left in the half-open [startHour, endHour) window, which may wrap
/// past midnight; 0 when outside it.
inline int secsUntilEnd(const std::tm& local, const int startHour, const int endHour)
{
    if (startHour == endHour)
        return 0;

    const int hour = local.tm_hour;
    const bool inside = (startHour < endHour)
        ? (hour >= startHour && hour < endHour)
        : (hour >= startHour || hour < endHour);
    if (!inside)
        return 0;

    constexpr int kDay = 24 * 60 * 60;
    const int now = hour * 3600 + local.tm_min * 60 + local.tm_sec;
    const int end = endHour * 3600;
    return end > now ? end - now : end + kDay - now;
}

/// Seconds until the window next opens; 0 when inside it or when it is empty.
inline int secsUntilStart(const std::tm& local, const int startHour, const int endHour)
{
    if (startHour == endHour || secsUntilEnd(local, startHour, endHour) > 0)
        return 0;

    constexpr int kDay = 24 * 60 * 60;
    const int now = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    const int start = startHour * 3600;
    return start > now ? start - now : start + kDay - now;
}
} // namespace night

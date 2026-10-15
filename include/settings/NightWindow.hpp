#pragma once
#include <ctime>

namespace night
{
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
} // namespace night

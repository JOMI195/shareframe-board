#pragma once
#include <ctime>

namespace daily
{
/// Seconds from `local` until the next full `hour`; a full day when it is that hour now.
inline int secsUntilHour(const std::tm& local, const int hour)
{
    constexpr int kDay = 24 * 60 * 60;
    const int now = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    const int diff = hour * 3600 - now;
    return diff > 0 ? diff : diff + kDay;
}
} // namespace daily

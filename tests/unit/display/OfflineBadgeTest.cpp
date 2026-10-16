#include "display/OfflineBadge.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr int64_t kNow = 1'760'000'000;
constexpr int kSixHours = 6 * 3600;

ServerConnection offlineFor(const int64_t secs) { return {false, kNow - secs, kNow - secs}; }
}

TEST(OfflineBadge, StaysAwayWhileConnectedOrBeforeTheThreshold)
{
    EXPECT_FALSE(offline::badgeText(kNow, {true, kNow - 999'999, kNow}, kSixHours));
    EXPECT_FALSE(offline::badgeText(kNow, offlineFor(kSixHours - 1), kSixHours));
    EXPECT_TRUE(offline::badgeText(kNow, offlineFor(kSixHours), kSixHours));
}

TEST(OfflineBadge, NeverShowsForAFrameThatNeverConnected)
{
    EXPECT_FALSE(offline::badgeText(kNow, {false, 0, 0}, kSixHours));
}

TEST(OfflineBadge, ThresholdZeroTurnsItOff)
{
    EXPECT_FALSE(offline::badgeText(kNow, offlineFor(999'999), 0));
}

TEST(OfflineBadge, SaysHowLongInMinutesHoursOrDays)
{
    EXPECT_EQ(offline::badgeText(kNow, offlineFor(5 * 60), 1), "Offline seit 5 Min.");
    EXPECT_EQ(offline::badgeText(kNow, offlineFor(7 * 3600 + 59 * 60), 1), "Offline seit 7 Std.");
    EXPECT_EQ(offline::badgeText(kNow, offlineFor(47 * 3600), 1), "Offline seit 47 Std.");
    EXPECT_EQ(offline::badgeText(kNow, offlineFor(3 * 86400 + 3600), 1), "Offline seit 3 Tagen");
}

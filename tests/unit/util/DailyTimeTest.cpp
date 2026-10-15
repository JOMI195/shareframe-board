#include "util/DailyTime.hpp"
#include <gtest/gtest.h>

namespace
{
std::tm at(int hour, int min = 0, int sec = 0)
{
    std::tm t{};
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    return t;
}
}

TEST(DailyTime, LaterTodayOrTomorrow)
{
    EXPECT_EQ(daily::secsUntilHour(at(1, 30), 2), 30 * 60);
    EXPECT_EQ(daily::secsUntilHour(at(2, 0, 1), 2), 24 * 3600 - 1);
    EXPECT_EQ(daily::secsUntilHour(at(23, 59, 59), 0), 1);
}

TEST(DailyTime, AtTheTargetHourWaitsAFullDay)
{
    EXPECT_EQ(daily::secsUntilHour(at(2), 2), 24 * 3600);
}

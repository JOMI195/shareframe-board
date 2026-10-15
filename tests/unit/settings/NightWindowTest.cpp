#include "settings/NightWindow.hpp"
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

TEST(NightWindow, SameDayWindowIsHalfOpen)
{
    EXPECT_EQ(night::secsUntilEnd(at(1, 59, 59), 2, 5), 0);
    EXPECT_EQ(night::secsUntilEnd(at(2), 2, 5), 3 * 3600);
    EXPECT_EQ(night::secsUntilEnd(at(4, 59, 59), 2, 5), 1);
    EXPECT_EQ(night::secsUntilEnd(at(5), 2, 5), 0);
}

TEST(NightWindow, WindowWrappingMidnight)
{
    EXPECT_EQ(night::secsUntilEnd(at(21, 59), 22, 6), 0);
    EXPECT_EQ(night::secsUntilEnd(at(22), 22, 6), 8 * 3600);
    EXPECT_EQ(night::secsUntilEnd(at(23, 30), 22, 6), 6 * 3600 + 30 * 60);
    EXPECT_EQ(night::secsUntilEnd(at(0), 22, 6), 6 * 3600);
    EXPECT_EQ(night::secsUntilEnd(at(5, 59, 59), 22, 6), 1);
    EXPECT_EQ(night::secsUntilEnd(at(6), 22, 6), 0);
    EXPECT_EQ(night::secsUntilEnd(at(12), 22, 6), 0);
}

TEST(NightWindow, EndAtMidnight)
{
    EXPECT_EQ(night::secsUntilEnd(at(23), 20, 0), 3600);
    EXPECT_EQ(night::secsUntilEnd(at(0), 20, 0), 0);
}

TEST(NightWindow, EqualStartAndEndIsAnEmptyWindow)
{
    for (int h = 0; h < 24; ++h)
        EXPECT_EQ(night::secsUntilEnd(at(h), 3, 3), 0) << h;
}

#include "display/ApSetupDebounce.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr int64_t kWindow = 600;
}

TEST(ApSetupDebounce, BootWithoutConnectionShowsSetupOnlyWithinWindow)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_FALSE(debounce.update("connecting", 10));
    EXPECT_TRUE(debounce.update("ap", 60));
    EXPECT_TRUE(debounce.update("connecting", 240)) << "station retry must not flap the panel";
    EXPECT_TRUE(debounce.update("ap", 300));
    EXPECT_FALSE(debounce.update("ap", kWindow));
    EXPECT_FALSE(debounce.update("connecting", kWindow + 180));
    EXPECT_FALSE(debounce.update("ap", kWindow + 240));
    EXPECT_FALSE(debounce.update("ap", 50000)) << "never again until reboot";
}

TEST(ApSetupDebounce, WindowExpiringDuringStationRetryHidesSetup)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_TRUE(debounce.update("ap", 60));
    EXPECT_FALSE(debounce.update("connecting", kWindow + 10));
}

TEST(ApSetupDebounce, OutageAfterConnectingKeepsPhotos)
{
    ApSetupDebounce debounce(kWindow);
    debounce.update("connected", 60);
    EXPECT_FALSE(debounce.update("connecting", 150));
    EXPECT_FALSE(debounce.update("ap", 200)) << "even inside the boot window";
    EXPECT_FALSE(debounce.update("ap", 5000));
}

TEST(ApSetupDebounce, AFrameThatNeverJoinedAWifiKeepsTheSetupScreenPastTheWindow)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_TRUE(debounce.update("ap", 60, false));
    EXPECT_TRUE(debounce.update("connecting", kWindow + 10, false)) << "a station retry later on";
    EXPECT_TRUE(debounce.update("ap", 50000, false)) << "an unboxed frame left alone for hours";
    EXPECT_FALSE(debounce.update("connected", 50010, false));
}

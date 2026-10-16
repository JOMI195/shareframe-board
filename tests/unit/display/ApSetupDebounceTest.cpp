#include "display/ApSetupDebounce.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr int64_t kWindow = 600;
}

TEST(ApSetupDebounce, FrameWithoutKnownNetworkAlwaysShowsSetupInAp)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_FALSE(debounce.update("connecting", false, 10));
    EXPECT_TRUE(debounce.update("ap", false, 60));
    EXPECT_TRUE(debounce.update("connecting", false, 240)) << "station retry must not flap the panel";
    EXPECT_FALSE(debounce.update("connected", false, 300));
    EXPECT_TRUE(debounce.update("ap", false, 5000));
}

TEST(ApSetupDebounce, BootWithoutConnectionShowsSetupOnlyWithinWindow)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_FALSE(debounce.update("connecting", true, 10));
    EXPECT_TRUE(debounce.update("ap", true, 60));
    EXPECT_TRUE(debounce.update("connecting", true, 240)) << "station retry must not flap the panel";
    EXPECT_TRUE(debounce.update("ap", true, 300));
    EXPECT_FALSE(debounce.update("ap", true, kWindow));
    EXPECT_FALSE(debounce.update("connecting", true, kWindow + 180));
    EXPECT_FALSE(debounce.update("ap", true, kWindow + 240));
}

TEST(ApSetupDebounce, WindowExpiringDuringStationRetryHidesSetup)
{
    ApSetupDebounce debounce(kWindow);
    EXPECT_TRUE(debounce.update("ap", true, 60));
    EXPECT_FALSE(debounce.update("connecting", true, kWindow + 10));
}

TEST(ApSetupDebounce, OutageAfterConnectingKeepsPhotos)
{
    ApSetupDebounce debounce(kWindow);
    debounce.update("connected", true, 60);
    EXPECT_FALSE(debounce.update("connecting", true, 150));
    EXPECT_FALSE(debounce.update("ap", true, 200)) << "even inside the boot window";
    EXPECT_FALSE(debounce.update("ap", true, 5000));
}

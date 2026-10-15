#include "display/ApSetupDebounce.hpp"
#include <gtest/gtest.h>

TEST(ApSetupDebounce, FrameWithoutKnownNetworkShowsSetupOnFirstAp)
{
    ApSetupDebounce debounce;
    EXPECT_FALSE(debounce.update("connecting"));
    EXPECT_TRUE(debounce.update("ap"));
    EXPECT_TRUE(debounce.update("connecting")) << "station retry must not flap the panel";
    EXPECT_TRUE(debounce.update("ap"));
    EXPECT_FALSE(debounce.update("connected"));
}

TEST(ApSetupDebounce, OutageShowsSetupOnlyAfterAFailedStationRetry)
{
    ApSetupDebounce debounce;
    debounce.update("connected");
    EXPECT_FALSE(debounce.update("ap"));
    EXPECT_FALSE(debounce.update("ap"));
    EXPECT_FALSE(debounce.update("connecting"));
    EXPECT_TRUE(debounce.update("ap"));

    debounce.update("connected");
    EXPECT_FALSE(debounce.update("ap")) << "a new outage starts the count again";
}

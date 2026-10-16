#include "util/WifiMode.hpp"
#include <gtest/gtest.h>

TEST(WifiMode, FactoryNetworksDontCountAsSaved)
{
    EXPECT_EQ(WifiMode::userNetworkCount({{"known_ssids", {"Jomi", "preconfigured"}}}), 0);
    EXPECT_EQ(WifiMode::userNetworkCount({{"known_ssids", {"jomi", "HomeNet"}}}), 1);
}

TEST(WifiMode, MissingSsidListCountsAsNoSavedNetwork)
{
    EXPECT_EQ(WifiMode::userNetworkCount({{"mode", "ap"}}), 0);
    EXPECT_EQ(WifiMode::userNetworkCount({{"known_ssids", "HomeNet"}}), 0);
}

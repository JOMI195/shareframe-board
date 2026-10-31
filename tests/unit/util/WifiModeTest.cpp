#include "util/WifiMode.hpp"
#include <gtest/gtest.h>

TEST(WifiMode, FactoryNetworksAreProtectedRegardlessOfCase)
{
    EXPECT_TRUE(WifiMode::isProtectedNetwork("preconfigured"));
    EXPECT_TRUE(WifiMode::isProtectedNetwork("jomi"));
    EXPECT_FALSE(WifiMode::isProtectedNetwork("HomeNet"));
}

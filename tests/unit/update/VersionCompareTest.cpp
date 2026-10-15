#include "update/UpdateManager.hpp"
#include <gtest/gtest.h>

TEST(VersionCompare, NumericSegments)
{
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.2.4", "1.2.3"));
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.10.0", "1.9.9"));
    EXPECT_TRUE(UpdateManager::isVersionNewer("2", "1.99"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.2.3", "1.2.3"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.2.2", "1.2.3"));
}

TEST(VersionCompare, MissingSegmentsCountAsZero)
{
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.2", "1.2.0"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.2.0", "1.2"));
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.2.0.1", "1.2"));
}

TEST(VersionCompare, LeadingZerosAreNumeric)
{
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.02", "1.2"));
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.010", "1.9"));
}

TEST(VersionCompare, DevBuildsAcceptAnyReleaseButNeverTheReverse)
{
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.0.0", "abc1234"));
    EXPECT_TRUE(UpdateManager::isVersionNewer("1.0.0", "dev"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("abc1234", "1.0.0"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("", "1.0.0"));
    EXPECT_FALSE(UpdateManager::isVersionNewer("1.0.0-rc1", "0.9"));
}

TEST(VersionCompare, HugeSegmentsDoNotThrow)
{
    EXPECT_NO_THROW({
        EXPECT_TRUE(UpdateManager::isVersionNewer("1.99999999999999999999999", "1.2"));
        EXPECT_FALSE(UpdateManager::isVersionNewer("1.2", "1.99999999999999999999999"));
        EXPECT_FALSE(UpdateManager::isVersionNewer("1.99999999999999999999999", "1.99999999999999999999999"));
    });
}

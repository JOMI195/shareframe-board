#include "util/HttpUtil.hpp"
#include <gtest/gtest.h>

TEST(HttpUtil, SplitsPathAndQuery)
{
    const auto [path, params] = HttpUtil::parseUri("/api/system/logs?service_name=display&lines=200");
    EXPECT_EQ(path, "/api/system/logs");
    EXPECT_EQ(params.at("service_name"), "display");
    EXPECT_EQ(params.at("lines"), "200");
}

TEST(HttpUtil, ToleratesSloppyQueries)
{
    const auto [path, params] = HttpUtil::parseUri("/x?&flag&a=b=c&=v&");
    EXPECT_EQ(path, "/x");
    EXPECT_EQ(params.at("flag"), "");
    EXPECT_EQ(params.at("a"), "b=c");
    EXPECT_EQ(HttpUtil::parseUri("/plain").second.size(), 0u);
}

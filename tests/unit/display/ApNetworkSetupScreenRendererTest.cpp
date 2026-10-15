#include "display/ApNetworkSetupScreenRenderer.hpp"
#include <gtest/gtest.h>

TEST(WifiQrPayload, SpecialCharactersAreEscaped)
{
    EXPECT_EQ(ApNetworkSetupScreenRenderer::wifiQrPayload(R"(Home;Net,"5:G")", R"(pa\ss;w:rd)"),
              R"(WIFI:T:WPA;S:Home\;Net\,\"5\:G\";P:pa\\ss\;w\:rd;;)");
}

TEST(WifiQrPayload, OpenNetworkUsesNopass)
{
    EXPECT_EQ(ApNetworkSetupScreenRenderer::wifiQrPayload("shareframe-board", ""),
              "WIFI:T:nopass;S:shareframe-board;;");
}

#include "dashboard/WifiManager.hpp"
#include "support/PathShim.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <gtest/gtest.h>

// The dashboard's Wi-Fi actions against shareframe-wifi / shareframe-ap stand-ins that record
// their argv one argument per line.
namespace
{
class WifiManagerTest : public ::testing::Test
{
protected:
    WifiManagerTest()
    {
        for (const auto* tool : {"shareframe-wifi", "shareframe-ap"})
            shims.add(tool, "for a in \"$@\"; do echo \"$a\"; done >> " + argv.string());
        shims.add("ping", "exit 0");
    }

    std::string calls() const
    {
        std::ifstream f(argv);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    PathShim shims;
    TempDir dir;
    std::filesystem::path argv = dir.path() / "argv";
    WifiManager wifi{(dir.path() / "wifi-mode.json").string()};
};
}

TEST_F(WifiManagerTest, AddingANetworkPassesSsidAndPasswordAsWholeArguments)
{
    EXPECT_TRUE(wifi.connect("my \"net\"; x", "pa ss$word").contains("message"));
    EXPECT_EQ(calls(), "add\nmy \"net\"; x\npa ss$word\n");
}

TEST_F(WifiManagerTest, AFailingAddIsReportedAsAnError)
{
    shims.add("shareframe-wifi", "echo 'FAIL' >&2; exit 1");
    EXPECT_EQ(wifi.connect("home", "home-pass-1").value("error", ""), "Failed to add WiFi network");
}

// The frame's factory networks keep it reachable for support: never changed or forgotten.
TEST_F(WifiManagerTest, FactoryNetworksCannotBeChangedOrForgotten)
{
    EXPECT_TRUE(wifi.connect("Preconfigured", "x-pass-123").contains("error"));
    EXPECT_TRUE(wifi.forget("preconfigured").contains("error"));
    EXPECT_EQ(calls(), "");
}

TEST_F(WifiManagerTest, TheNetworkTheBoardIsOnCannotBeForgotten)
{
    shims.add("shareframe-wifi", "[ \"$1\" = status ] && printf 'ssid=home\\nwpa_state=COMPLETED\\n'; "
                                 "[ \"$1\" = del ] && echo del >> " + argv.string() + "; exit 0");
    EXPECT_EQ(wifi.forget("home").value("error", ""), "Active network");
    EXPECT_TRUE(wifi.forget("office").contains("message"));
    EXPECT_EQ(calls(), "del\n");
}

TEST_F(WifiManagerTest, TheHotspotPasswordIsPassedWholeAndAFailureReported)
{
    EXPECT_TRUE(wifi.setApPassword("it's \"x\" $y").contains("message"));
    EXPECT_EQ(calls(), "setpass\nit's \"x\" $y\n");
    shims.add("shareframe-ap", "echo 'setpass: password must be 8-63 bytes' >&2; exit 1");
    EXPECT_EQ(wifi.setApPassword("short").value("error", ""), "Failed to set AP password");
}

// wifi-mode-manager only looks for the retry request while it hosts the hotspot.
TEST_F(WifiManagerTest, ARetryRequestIsLeftNextToTheWifiMode)
{
    EXPECT_TRUE(wifi.requestConnect().contains("message"));
    EXPECT_TRUE(std::filesystem::exists(dir.path() / "wifi-retry-requested"));
}

TEST_F(WifiManagerTest, TheNetworkTheBoardIsOnShowsByItsRealName)
{
    shims.add("shareframe-wifi", "[ \"$1\" = status ] && printf 'ssid=M\\\\xc3\\\\xbcller WLAN\\nwpa_state=COMPLETED\\n'");
    EXPECT_EQ(wifi.getCurrentConnection().at("connection_name"), "Müller WLAN");
}

#include "dashboard/WifiManager.hpp"
#include "support/Golden.hpp"
#include "support/PathShim.hpp"
#include "support/TempDir.hpp"
#include "util/WifiMode.hpp"
#include <gtest/gtest.h>

namespace
{
class WifiContractTest : public ::testing::Test
{
protected:
    WifiContractTest()
    {
        shims.add("shareframe-wifi", "case \"$1\" in list) cat '" + golden::hardware("wifi-list.txt").string() +
                                     "' ;; status) cat '" + golden::hardware("wifi-status.txt").string() + "' ;; esac");
    }

    PathShim shims;
    TempDir dir;
    WifiManager wifi{(dir.path() / "wifi-mode.json").string()};
};
}

// SSIDs stay in wpa_cli's encoding: forgetting a network sends back exactly what was listed.
TEST_F(WifiContractTest, SavedNetworksAreTheListedSsidsInListOrder)
{
    const auto networks = wifi.getSavedNetworks().at("networks");
    EXPECT_EQ(networks, (nlohmann::json{"home", "Caf\\xc3\\xa9 Wi-Fi", "my net", "5", "quote\\\"d"}));
}

TEST_F(WifiContractTest, CurrentConnectionIsTheAssociatedSsid)
{
    EXPECT_EQ(wifi.getCurrentConnection().at("connection_name"), "my net");
}

TEST(WifiModeContract, ConnectedModeCarriesItsNetworkAndInternet)
{
    const auto mode = WifiMode::read(golden::hardware("wifi-mode-connected.json").string());
    EXPECT_EQ(mode.at("mode"), "connected");
    EXPECT_EQ(mode.at("ssid"), "home");
    EXPECT_EQ(mode.at("internet"), true);
    EXPECT_EQ(mode.at("known_ssids"), nlohmann::json{"home"});
}

// The setup screen's QR code and the dashboard show these credentials verbatim.
TEST(WifiModeContract, HotspotCredentialsArriveVerbatim)
{
    const auto mode = WifiMode::read(golden::hardware("wifi-mode-ap.json").string());
    EXPECT_EQ(mode.at("mode"), "ap");
    EXPECT_EQ(mode.at("ap_ssid"), "frame \"one\"");
    EXPECT_EQ(mode.at("ap_password"), "pa\"ss\\word's");
    EXPECT_GT(mode.at("last_failed_at").get<int64_t>(), 0);
}

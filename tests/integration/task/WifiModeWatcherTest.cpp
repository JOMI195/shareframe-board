#include "support/ImageFixture.hpp"
#include "support/TempDir.hpp"
#include "support/Wait.hpp"
#include "task/WifiModeWatcher.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace std::chrono_literals;

// What the display hears about the hotspot from the Wi-Fi daemon's mode file.
namespace
{
class WifiModeWatcherTest : public ::testing::Test
{
protected:
    // The debounce takes the boot window when the watcher is built; the host's uptime is long.
    static AppConfig config(const TempDir& dir)
    {
        AppConfig cfg;
        cfg.display.wifiModePath = (dir.path() / "wifi-mode.json").string();
        cfg.display.apSetupBootWindowSecs = 2000000000;
        return cfg;
    }

    ~WifiModeWatcherTest() override
    {
        watcher.stop();
        bus.stop();
    }

    void mode(const nlohmann::json& m) const
    {
        const auto tmp = cfg.display.wifiModePath + ".tmp";
        std::ofstream(tmp) << m.dump();
        std::filesystem::rename(tmp, cfg.display.wifiModePath);
    }

    void start()
    {
        watcher.start();
        bus.start();
    }

    TempDir dir;
    AppConfig cfg = config(dir);
    EventBus bus;
    Recorder<Topic::AP_SETUP_STATE> states{bus};
    WifiModeWatcher watcher{bus, cfg};
};
}

TEST_F(WifiModeWatcherTest, TheHotspotWithItsCredentialsAsksForTheSetupScreen)
{
    mode({{"mode", "ap"}, {"ap_ssid", "frame"}, {"ap_password", "hotspot-1"}});
    start();
    ASSERT_TRUE(waitFor([&] { return states.size() == 1; }));
    EXPECT_EQ(states.get()[0], (ApSetupStateEvent{true, "frame", "hotspot-1"}));
}

TEST_F(WifiModeWatcherTest, ConnectingAfterwardsEndsItAndAnUnchangedModeIsNotRepeated)
{
    mode({{"mode", "ap"}, {"ap_ssid", "frame"}, {"ap_password", "hotspot-1"}});
    start();
    ASSERT_TRUE(waitFor([&] { return states.size() == 1; }));
    watcher.runNow();
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(states.size(), 1u);

    mode({{"mode", "connected"}, {"ssid", "home"}});
    watcher.runNow();
    ASSERT_TRUE(waitFor([&] { return states.size() == 2; }));
    EXPECT_FALSE(states.get()[1].required);
}

// A QR code without an SSID would send the owner nowhere.
TEST_F(WifiModeWatcherTest, AHotspotWithoutAnSsidKeepsTheCurrentScreen)
{
    mode({{"mode", "connected"}, {"ssid", "home"}});
    mode({{"mode", "ap"}, {"ap_password", "hotspot-1"}});
    start();
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(states.size(), 0u);
}

TEST_F(WifiModeWatcherTest, AMissingOrBrokenFileIsNoHotspot)
{
    start();
    std::this_thread::sleep_for(300ms);
    std::ofstream(cfg.display.wifiModePath) << "{\"mode\": \"ap\", ";
    watcher.runNow();
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(states.size(), 0u);
}

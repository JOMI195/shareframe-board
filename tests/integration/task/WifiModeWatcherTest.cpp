#include "support/ImageFixture.hpp"
#include "support/TempDir.hpp"
#include "support/Wait.hpp"
#include "task/WifiModeWatcher.hpp"
#include "util/PanelCode.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
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
        cfg.display.setupCodePath = (dir.path() / "setup-code").string();
        cfg.display.wifiJoinedPath = (dir.path() / "wifi-joined").string();
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
    const auto code = panel_code::read(cfg.display.setupCodePath);
    ASSERT_EQ(code.size(), 6u);
    EXPECT_EQ(states.get()[0], (ApSetupStateEvent{true, "frame", "hotspot-1", code}));
}

TEST_F(WifiModeWatcherTest, ARestartedDisplayShowsTheCodeThePanelAlreadyHas)
{
    std::ofstream(cfg.display.setupCodePath) << "482913\n";
    mode({{"mode", "ap"}, {"ap_ssid", "frame"}, {"ap_password", "hotspot-1"}});
    start();
    ASSERT_TRUE(waitFor([&] { return states.size() == 1; }));
    EXPECT_EQ(states.get()[0].code, "482913");
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

class WifiModeWatcherWindowTest : public WifiModeWatcherTest
{
protected:
    WifiModeWatcherWindowTest()
    {
        cfg.display.apSetupBootWindowSecs = 0;
        windowed.emplace(bus, cfg);
    }
    std::optional<WifiModeWatcher> windowed;
};

TEST_F(WifiModeWatcherWindowTest, AFrameThatNeverJoinedAWifiAsksForSetupPastTheBootWindow)
{
    mode({{"mode", "ap"}, {"ap_ssid", "frame"}, {"ap_password", "hotspot-1"}});
    windowed->start();
    bus.start();
    EXPECT_TRUE(waitFor([&] { return states.size() == 1 && states.get()[0].required; }));
    windowed->stop();
}

TEST_F(WifiModeWatcherWindowTest, AFrameSetUpBeforeKeepsItsPhotosPastTheBootWindow)
{
    std::ofstream(cfg.display.wifiJoinedPath) << "1700000000\n";
    mode({{"mode", "ap"}, {"ap_ssid", "frame"}, {"ap_password", "hotspot-1"}});
    windowed->start();
    bus.start();
    std::this_thread::sleep_for(1500ms);
    windowed->stop();
    for (const auto& s : states.get())
        EXPECT_FALSE(s.required);
}

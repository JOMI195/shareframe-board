#include "repository/DisplayMetricsRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include "display/LogoScreenRenderer.hpp"
#include "task/DisplayTaskManager.hpp"
#include <fstream>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class DisplayTaskManagerTest : public ::testing::Test, protected ImageFixture
{
protected:
    DisplayTaskManagerTest()
    {
        cfg.display.mockDisplay = true;
        cfg.display.minRefreshSecs = 0;
        cfg.display.mockOutputDir = (db.dir() / "panel").string();
        cfg.display.loadingImagePath = std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading";
        cfg.display.defaultImagesPath = std::string(SHAREFRAME_ASSETS_DIR) + "/image/default";
        cfg.display.setupFontPath = std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans.ttf";
        cfg.display.boldFontPath = std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans-Bold.ttf";
        // The migrated DB blanks the panel from 1 to 6, whatever the wall clock says here.
        settings.setNightMode(false, 1, 6, 3600, false);
    }

    void TearDown() override
    {
        manager.stop();
        bus.stop();
    }

    void start()
    {
        loop.subscribe();
        manager.subscribe();
        manager.start();
        bus.start();
    }

    void apSetup(const bool required, const std::string& password = "hotspot123")
    {
        bus.publish<Topic::AP_SETUP_STATE>({required, required ? "shareframe-board" : "", required ? password : ""});
    }

    void status(const std::optional<StatusScreenKind> kind, const std::string& version = "")
    {
        bus.publish<Topic::STATUS_SCREEN>({kind, version});
    }

    bool showsBoot() const { return source().ends_with(LogoScreenRenderer::IMAGE); }

    nlohmann::json panel() const
    {
        std::ifstream f(std::filesystem::path(cfg.display.mockOutputDir) / "panel.json");
        try
        {
            if (f)
                return nlohmann::json::parse(f);
        }
        catch (...)
        {
        }
        return nlohmann::json::object();
    }

    std::string source() const { return panel().value("source", ""); }
    int64_t seq() const { return panel().value("seq", int64_t{0}); }
    bool showsDefault() const { return source().find("/image/default/") != std::string::npos; }
    bool showsNight() const { return source() == NightBlankScreen::SOURCE; }

    // A window starting this hour, so the night is active whatever the wall clock says.
    std::pair<int, int> nightNow(const int hours = 1) const
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_r(&now, &local);
        return {local.tm_hour, (local.tm_hour + hours) % 24};
    }

    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
    ApNetworkSetupScreen setupScreen{bus, cfg, display};
    NightBlankScreen nightBlank{bus, cfg, display};
    StatusScreen statusScreen{bus, cfg, display};
    DisplayTaskManager manager{bus, cfg, display, loop, setupScreen, nightBlank, statusScreen, settings};
};
}

TEST_F(DisplayTaskManagerTest, BootsIntoTheSlideshow)
{
    start();
    EXPECT_TRUE(waitFor([&] { return showsDefault(); }));
    EXPECT_EQ(manager.state(), DisplayState::Slideshow);
}

// The pending photo must never reach the panel: it would cost a refresh and hide the QR.
TEST_F(DisplayTaskManagerTest, SetupScreenReplacesPendingPhotoAtOnce)
{
    const auto picture = db.dir() / "user.jpg";
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/default/sample_2.jpg", picture);
    repo.save({1, "alice", picture, std::time(nullptr) + 3600});
    cfg.display.minRefreshSecs = 1;
    start();
    ASSERT_TRUE(waitFor([&] { return manager.state() == DisplayState::Slideshow; }));
    ASSERT_TRUE(showsBoot());

    apSetup(true);
    ASSERT_TRUE(waitFor([&] { return source() == "ap-setup"; }, 500ms)) << "setup screen waited for the refresh interval";
    const auto shownAt = seq();

    std::this_thread::sleep_for(1500ms);
    EXPECT_EQ(seq(), shownAt) << "the photo pending at the switch was painted";
}

TEST_F(DisplayTaskManagerTest, NewApPasswordRepaintsTheSetupScreen)
{
    start();
    apSetup(true, "first-pass");
    ASSERT_TRUE(waitFor([&] { return source() == "ap-setup"; }));
    const auto first = seq();

    apSetup(true, "second-pass");
    EXPECT_TRUE(waitFor([&] { return seq() > first; }));
    EXPECT_EQ(source(), "ap-setup");
}

TEST_F(DisplayTaskManagerTest, ConnectingReturnsToTheSlideshow)
{
    start();
    apSetup(true);
    ASSERT_TRUE(waitFor([&] { return source() == "ap-setup"; }));

    apSetup(false);
    EXPECT_TRUE(waitFor([&] { return showsDefault(); }));
    EXPECT_EQ(manager.state(), DisplayState::Slideshow);
}

TEST_F(DisplayTaskManagerTest, StoppedSlideshowGetsABlankPanelInsteadOfAStaleQr)
{
    settings.setSlideshowActive(false);
    start();
    apSetup(true);
    ASSERT_TRUE(waitFor([&] { return source() == "ap-setup"; }));

    apSetup(false);
    EXPECT_TRUE(waitFor([&] { return panel().value("state", "") == "cleared"; }));
}

TEST_F(DisplayTaskManagerTest, BlankNightPausesTheSlideshowAndYieldsOnlyToTheSetupScreen)
{
    start();
    ASSERT_TRUE(waitFor([&] { return showsDefault(); }));

    const auto [from, to] = nightNow();
    bus.publish<Topic::UPDATE_NIGHT_MODE>({true, from, to, 3600, true});
    ASSERT_TRUE(waitFor([&] { return showsNight(); }));
    EXPECT_EQ(manager.state(), DisplayState::NightBlank);
    const auto blankedAt = seq();

    bus.publish<Topic::SKIP_IMAGE>({});
    std::this_thread::sleep_for(500ms);
    EXPECT_EQ(seq(), blankedAt) << "panel repainted during a blank night";

    apSetup(true);
    ASSERT_TRUE(waitFor([&] { return source() == "ap-setup"; }));
    apSetup(false);
    ASSERT_TRUE(waitFor([&] { return showsNight(); }));
    EXPECT_EQ(manager.state(), DisplayState::NightBlank);

    bus.publish<Topic::UPDATE_NIGHT_MODE>({false, from, to, 3600, true});
    EXPECT_TRUE(waitFor([&] { return showsDefault(); }));
    EXPECT_EQ(manager.state(), DisplayState::Slideshow);
}

TEST_F(DisplayTaskManagerTest, NewNightEndRepaintsTheNightScreen)
{
    start();
    const auto [from, to] = nightNow();
    bus.publish<Topic::UPDATE_NIGHT_MODE>({true, from, to, 3600, true});
    ASSERT_TRUE(waitFor([&] { return showsNight(); }));
    const auto shownAt = seq();

    bus.publish<Topic::UPDATE_NIGHT_MODE>({true, from, nightNow(2).second, 3600, true});
    EXPECT_TRUE(waitFor([&] { return seq() > shownAt; }));
    EXPECT_TRUE(showsNight());
}

TEST_F(DisplayTaskManagerTest, NightEndingOnAStoppedSlideshowClearsTheNightScreen)
{
    settings.setSlideshowActive(false);
    start();
    const auto [from, to] = nightNow();
    bus.publish<Topic::UPDATE_NIGHT_MODE>({true, from, to, 3600, true});
    ASSERT_TRUE(waitFor([&] { return showsNight(); }));

    bus.publish<Topic::UPDATE_NIGHT_MODE>({false, from, to, 3600, true});
    EXPECT_TRUE(waitFor([&] { return panel().value("state", "") == "cleared"; }));
}

TEST_F(DisplayTaskManagerTest, StatusScreenOutranksTheSetupScreen)
{
    start();
    ASSERT_TRUE(waitFor([&] { return showsDefault(); }));

    status(StatusScreenKind::Update, "1.3.0");
    ASSERT_TRUE(waitFor([&] { return source() == "status:update"; }));
    EXPECT_EQ(manager.state(), DisplayState::Status);

    apSetup(true);
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(source(), "status:update");

    status(std::nullopt);
    EXPECT_TRUE(waitFor([&] { return source() == "ap-setup"; }));
}

TEST_F(DisplayTaskManagerTest, HiddenStatusScreenReturnsToTheSlideshow)
{
    start();
    status(StatusScreenKind::Reboot);
    ASSERT_TRUE(waitFor([&] { return source() == "status:reboot"; }));

    status(std::nullopt);
    EXPECT_TRUE(waitFor([&] { return showsDefault(); }));
    EXPECT_EQ(manager.state(), DisplayState::Slideshow);
}

TEST_F(DisplayTaskManagerTest, AnotherStatusScreenReplacesTheShownOne)
{
    start();
    status(StatusScreenKind::Reboot);
    ASSERT_TRUE(waitFor([&] { return source() == "status:reboot"; }));

    status(StatusScreenKind::FactoryReset);
    EXPECT_TRUE(waitFor([&] { return source() == "status:factory_reset"; }));
}

TEST_F(DisplayTaskManagerTest, StatusScreenWaitsForTheRefreshInterval)
{
    cfg.display.minRefreshSecs = 2;
    start();
    ASSERT_TRUE(waitFor([&] { return showsBoot(); }));
    const auto bootSeq = seq();

    status(StatusScreenKind::Update, "1.3.0");
    std::this_thread::sleep_for(500ms);
    EXPECT_EQ(seq(), bootSeq) << "status screen painted inside the refresh interval";
    EXPECT_TRUE(waitFor([&] { return source() == "status:update"; }, 3000ms));
}

TEST_F(DisplayTaskManagerTest, StatusScreenHiddenWhileWaitingIsNeverPainted)
{
    cfg.display.minRefreshSecs = 1;
    start();
    ASSERT_TRUE(waitFor([&] { return showsBoot(); }));
    const auto bootSeq = seq();

    status(StatusScreenKind::Reboot);
    std::this_thread::sleep_for(200ms);
    status(std::nullopt);
    ASSERT_TRUE(waitFor([&] { return showsDefault(); }, 3000ms));
    EXPECT_EQ(seq(), bootSeq + 1) << "the status screen was painted";
}

TEST_F(DisplayTaskManagerTest, ForgottenStatusScreenTimesOut)
{
    cfg.display.statusScreenTimeoutSecs = 1;
    start();
    status(StatusScreenKind::Reboot);
    ASSERT_TRUE(waitFor([&] { return source() == "status:reboot"; }));
    EXPECT_TRUE(waitFor([&] { return showsDefault(); }, 3000ms));
}

TEST_F(DisplayTaskManagerTest, ShutdownPaintsAStatusScreenStillWaitingForTheInterval)
{
    cfg.display.minRefreshSecs = 3600;
    start();
    ASSERT_TRUE(waitFor([&] { return showsBoot(); }));
    status(StatusScreenKind::Reboot);
    ASSERT_TRUE(waitFor([&] { return manager.state() == DisplayState::Status; }));

    display.requestShutdown();
    manager.stop();
    statusScreen.finishForShutdown();
    EXPECT_EQ(source(), "status:reboot");
}

TEST_F(DisplayTaskManagerTest, ShutdownKeepsAShownStatusScreenWithoutRepainting)
{
    start();
    status(StatusScreenKind::Reboot);
    ASSERT_TRUE(waitFor([&] { return source() == "status:reboot"; }));
    const auto shownAt = seq();

    display.requestShutdown();
    manager.stop();
    statusScreen.finishForShutdown();
    EXPECT_EQ(seq(), shownAt);
}

#include "repository/DisplayMetricsRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
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

    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
    ApNetworkSetupScreen setupScreen{bus, cfg, display};
    DisplayTaskManager manager{bus, cfg, display, loop, setupScreen, settings};
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
    ASSERT_TRUE(source().ends_with("logo-frame-loading-shareframe.jpg"));

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

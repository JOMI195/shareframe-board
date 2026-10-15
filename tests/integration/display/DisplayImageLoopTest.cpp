#include "display/DisplayManager.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include "task/DisplayImageLoop.hpp"
#include <fstream>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class DisplayImageLoopTest : public ::testing::Test, protected ImageFixture
{
protected:
    DisplayImageLoopTest()
    {
        cfg.display.mockDisplay = true;
        cfg.display.minRefreshSecs = 0;
        cfg.display.mockOutputDir = (db.dir() / "panel").string();
        cfg.display.defaultImagesPath = std::string(SHAREFRAME_ASSETS_DIR) + "/image/default";
    }

    void TearDown() override
    {
        loop.stop();
        bus.stop();
    }

    void start()
    {
        loop.subscribe();
        loop.start();
        bus.start();
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

    int64_t seq() const { return panel().value("seq", int64_t{0}); }

    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
};
}

TEST_F(DisplayImageLoopTest, ShowsDefaultImagesWithoutUserPictures)
{
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").find("/image/default/") != std::string::npos; }));
    EXPECT_TRUE(loop.isReady());
}

TEST_F(DisplayImageLoopTest, SkipAdvancesToNextDefaultImage)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));
    const auto first = panel();
    bus.publish<Topic::SKIP_IMAGE>({});
    ASSERT_TRUE(waitFor([&] { return seq() > first["seq"].get<int64_t>(); }));
    EXPECT_NE(panel()["source"], first["source"]);
}

TEST_F(DisplayImageLoopTest, StoppedSlideshowLeavesDefaultImageAlone)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));

    bus.publish<Topic::SET_SLIDESHOW_ACTIVE>({false});
    ASSERT_TRUE(waitFor([&] { return !settings.isSlideshowActive(); }));
    std::this_thread::sleep_for(200ms);
    const auto stoppedAt = seq();

    std::this_thread::sleep_for(1s);
    EXPECT_EQ(seq(), stoppedAt) << "panel repainted while the slideshow was stopped";
    EXPECT_EQ(loop.secondsUntilNext(), -1);
}

TEST_F(DisplayImageLoopTest, UserPictureReplacesDefaultImage)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));

    const auto picture = db.dir() / "user.jpg";
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-loading-shareframe.jpg", picture);
    repo.save({42, "alice", picture, std::time(nullptr) + 3600});
    bus.publish<Topic::IMAGE_NEW>(42);

    EXPECT_TRUE(waitFor([&] { return panel().value("source", "") == picture.string(); }));
}

TEST_F(DisplayImageLoopTest, RemovingTheShownPictureMovesOn)
{
    for (const int64_t id : {1, 2})
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-loading-shareframe.jpg", path);
        repo.save({id, "alice", path, std::time(nullptr) + 3600});
    }
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/1.jpg"); }));

    repo.remove(1);
    bus.publish<Topic::IMAGE_REMOVED>({1});
    EXPECT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/2.jpg"); }));
}

TEST_F(DisplayImageLoopTest, PlaybackOrderDecidesWhichPictureComesNext)
{
    for (const auto& [id, receivedAt] : std::vector<std::pair<int64_t, int64_t>>{{1, 100}, {2, 300}, {3, 200}})
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-loading-shareframe.jpg", path);
        repo.save({id, "alice", path, std::time(nullptr) + 3600, receivedAt});
    }
    settings.setPlaybackOrder(PlaybackMode::NewestFirst, {});
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/2.jpg"); }));
    EXPECT_EQ(loop.currentImageId(), 2);

    bus.publish<Topic::SKIP_IMAGE>({});
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/3.jpg"); }));

    bus.publish<Topic::UPDATE_PLAYBACK_ORDER>({PlaybackMode::Custom, {3, 2, 1}});
    ASSERT_TRUE(waitFor([&] { return settings.getPlaybackMode() == PlaybackMode::Custom; }));
    bus.publish<Topic::SKIP_IMAGE>({});
    EXPECT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/2.jpg"); }));
}

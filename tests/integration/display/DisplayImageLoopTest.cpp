#include "display/DisplayManager.hpp"
#include "display/PanelBadge.hpp"
#include "display/PhotoRenderer.hpp"
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
        // The migrated DB holds new pictures back from 1 to 6, whatever the wall clock says here.
        settings.setNightMode(false, 1, 6, 3600, NightStyle::Slower);
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
    std::string source() const { return panel().value("source", ""); }
    bool cleared() const { return panel().value("state", "") == "cleared"; }

    std::vector<uint8_t> shownBuffer() const
    {
        std::ifstream f(std::filesystem::path(cfg.display.mockOutputDir) / "panel.pbm", std::ios::binary);
        const std::string pbm{std::istreambuf_iterator<char>(f), {}};
        // PBM stores black as 1, the panel buffer as 0.
        std::vector<uint8_t> shown(pbm.end() - DisplayManager::BUFFER_SIZE, pbm.end());
        for (auto& byte : shown)
            byte = ~byte;
        return shown;
    }

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
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", picture);
    repo.save({42, "alice", picture, std::time(nullptr) + 3600});
    bus.publish<Topic::IMAGE_NEW>(42);

    EXPECT_TRUE(waitFor([&] { return panel().value("source", "") == picture.string(); }));
}

TEST_F(DisplayImageLoopTest, RemovingTheShownPictureMovesOn)
{
    for (const int64_t id : {1, 2})
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", path);
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
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", path);
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

TEST_F(DisplayImageLoopTest, ShowNowPaintsTheRequestedPictureAndContinuesFromIt)
{
    for (const int64_t id : {1, 2, 3})
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", path);
        repo.save({id, "alice", path, std::time(nullptr) + 3600, id * 100});
    }
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/1.jpg"); }));

    bus.publish<Topic::SHOW_IMAGE>({3});
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/3.jpg"); }));
    EXPECT_EQ(loop.currentImageId(), 3);

    bus.publish<Topic::SKIP_IMAGE>({});
    EXPECT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/1.jpg"); }));
}

TEST_F(DisplayImageLoopTest, ShowNowOfAVanishedPictureFallsBackToTheNextOne)
{
    for (const int64_t id : {1, 2})
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", path);
        repo.save({id, "alice", path, std::time(nullptr) + 3600, id * 100});
    }
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/1.jpg"); }));

    bus.publish<Topic::SHOW_IMAGE>({99});
    EXPECT_TRUE(waitFor([&] { return panel().value("source", "").ends_with("/2.jpg"); }));
}

TEST_F(DisplayImageLoopTest, HeldPhotosCarryANightBadge)
{
    cfg.display.boldFontPath = std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans-Bold.ttf";
    const auto picture = db.dir() / "user.jpg";
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/default/sample_2.jpg", picture);
    repo.save({42, "alice", picture, std::time(nullptr) + 3600});
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    const int endHour = (local.tm_hour + 2) % 24;
    settings.setNightMode(true, local.tm_hour, endHour, 3600, NightStyle::Hold);
    start();
    ASSERT_TRUE(waitFor([&] { return panel().value("source", "") == picture.string(); }));
    loop.stop();

    const auto shown = shownBuffer();
    const auto plain = PhotoRenderer::render(picture);
    auto badged = plain;
    panel_badge::draw(badged, cfg.display.boldFontPath, panel_badge::nightText(endHour),
                      panel_badge::Corner::BottomRight);
    EXPECT_NE(shown, plain);
    EXPECT_EQ(shown, badged);
}

TEST_F(DisplayImageLoopTest, SenderBadgeOnlyNamesWhoSentTheUserPicture)
{
    cfg.display.boldFontPath = std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans-Bold.ttf";
    start();
    bus.publish<Topic::UPDATE_CUSTOMIZATION>({true});
    ASSERT_TRUE(waitFor([&] { return settings.isShowSender() && seq() >= 1; }));
    const auto defaultImage = source();
    EXPECT_EQ(shownBuffer(), PhotoRenderer::render(defaultImage));

    const auto picture = db.dir() / "user.jpg";
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/default/sample_2.jpg", picture);
    repo.save({42, "alice", picture, std::time(nullptr) + 3600});
    bus.publish<Topic::IMAGE_NEW>(42);
    ASSERT_TRUE(waitFor([&] { return source() == picture.string(); }));
    loop.stop();

    auto badged = PhotoRenderer::render(picture);
    panel_badge::draw(badged, cfg.display.boldFontPath, "alice", panel_badge::Corner::TopRight);
    EXPECT_EQ(shownBuffer(), badged);
}

TEST_F(DisplayImageLoopTest, ReportsRefreshingOnlyWhilePainting)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));
    EXPECT_TRUE(waitFor([&] { return !loop.isRefreshing(); }));
}

TEST_F(DisplayImageLoopTest, PaintWaitingForThePanelCountsAsRefreshing)
{
    cfg.display.minRefreshSecs = 3600;
    const auto picture = db.dir() / "user.jpg";
    std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg", picture);
    repo.save({42, "alice", picture, std::time(nullptr) + 3600});
    start();

    EXPECT_TRUE(waitFor([&] { return loop.isRefreshing(); }));
    EXPECT_EQ(seq(), 0);
}

TEST_F(DisplayImageLoopTest, DailyClearRepaintsTheCurrentPictureAfterTheRefreshInterval)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));
    const auto shown = source();

    display.clear();
    cfg.display.minRefreshSecs = 1;
    bus.publish<Topic::SCHEDULED_CLEAR>({});
    std::this_thread::sleep_for(300ms);
    EXPECT_TRUE(cleared()) << "repainted inside the refresh interval";
    EXPECT_TRUE(waitFor([&] { return !cleared() && seq() > 0; }, 3000ms));
    EXPECT_EQ(source(), shown);
}

TEST_F(DisplayImageLoopTest, DailyClearLeavesTheRepaintToAPictureDueFirst)
{
    settings.setDisplayInterval(2);
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));
    const auto shown = source();

    display.clear();
    cfg.display.minRefreshSecs = 4;
    bus.publish<Topic::SCHEDULED_CLEAR>({});
    ASSERT_TRUE(waitFor([&] { return !cleared(); }, 6000ms));
    EXPECT_NE(source(), shown);
}

TEST_F(DisplayImageLoopTest, DailyClearLeavesAStoppedSlideshowBlank)
{
    start();
    ASSERT_TRUE(waitFor([&] { return seq() >= 1; }));
    bus.publish<Topic::SET_SLIDESHOW_ACTIVE>({false});
    ASSERT_TRUE(waitFor([&] { return !settings.isSlideshowActive(); }));

    display.clear();
    bus.publish<Topic::SCHEDULED_CLEAR>({});
    std::this_thread::sleep_for(500ms);
    EXPECT_TRUE(cleared());
}

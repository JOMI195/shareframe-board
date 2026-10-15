#include "dashboard/FrameHandlers.hpp"
#include "dashboard/SlideshowImageHandlers.hpp"
#include "display/DisplayIpc.hpp"
#include "ipc/NngRepServer.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/Http.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

// Dashboard HTTP handler → nng → display service → display bus, for every frame action.
namespace
{
class DashboardDisplayTest : public ::testing::Test, protected ImageFixture
{
protected:
    DashboardDisplayTest()
    {
        cfg.display.mockDisplay = true;
        bus.start();
        server.start();
    }

    ~DashboardDisplayTest() override
    {
        server.stop();
        bus.stop();
    }

    std::string url = "ipc://" + (db.dir() / "display.sock").string();
    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
    DisplayIpc displayIpc{bus, settings, loop, repo, display};

    Recorder<Topic::SKIP_IMAGE> skips{bus};
    Recorder<Topic::SET_SLIDESHOW_ACTIVE> active{bus};
    Recorder<Topic::UPDATE_DISPLAY_INTERVAL> intervals{bus};
    Recorder<Topic::UPDATE_NIGHT_MODE> nightModes{bus};
    Recorder<Topic::CLEAR_DISPLAY> clears{bus};
    Recorder<Topic::UPDATE_PLAYBACK_ORDER> orders{bus};

    NngRepServer server{url, [this](const IpcMessage& m) { return displayIpc.handle(m); }};
    IpcClient ipc{url};
    FrameHandlers frame{ipc, cfg};
    SlideshowImageHandlers slideshowImages{ipc, repo};

    std::filesystem::path picture(const int64_t id, const int64_t receivedAt)
    {
        const auto path = db.dir() / (std::to_string(id) + ".jpg");
        std::filesystem::copy_file(std::string(SHAREFRAME_ASSETS_DIR) + "/image/default/sample_2.jpg", path);
        repo.save({id, "sender" + std::to_string(id), path, 5000 + id, receivedAt});
        return path;
    }
};
}

TEST_F(DashboardDisplayTest, SkipReachesTheLoop)
{
    EXPECT_EQ(result(frame.handleSkip(httpRequest("POST", "/"))).status, 200);
    EXPECT_TRUE(waitFor([&] { return skips.size() == 1; }));
}

TEST_F(DashboardDisplayTest, StartAndStopSetSlideshowActive)
{
    EXPECT_EQ(result(frame.handleControl(httpRequest("POST", "/", {{"action", "stop"}}))).status, 200);
    EXPECT_EQ(result(frame.handleControl(httpRequest("POST", "/", {{"action", "start"}}))).status, 200);
    ASSERT_TRUE(waitFor([&] { return active.size() == 2; }));
    EXPECT_FALSE(active.get()[0].active);
    EXPECT_TRUE(active.get()[1].active);
}

TEST_F(DashboardDisplayTest, IntervalInSecondsArrivesUnchanged)
{
    EXPECT_EQ(result(frame.handleUpdateInterval(httpRequest("POST", "/", {{"interval_seconds", 1800}}))).status, 200);
    ASSERT_TRUE(waitFor([&] { return intervals.size() == 1; }));
    EXPECT_EQ(intervals.get()[0].intervalSecs, 1800);
}

TEST_F(DashboardDisplayTest, NightModeFieldsArriveUnchanged)
{
    const auto r = result(frame.handleUpdateNightMode(httpRequest("POST", "/",
        {{"enabled", true}, {"start_hour", 22}, {"end_hour", 6}, {"interval_seconds", 7200}, {"blank", true}})));
    EXPECT_EQ(r.status, 200);
    ASSERT_TRUE(waitFor([&] { return nightModes.size() == 1; }));
    const auto e = nightModes.get()[0];
    EXPECT_TRUE(e.enabled);
    EXPECT_EQ(e.startHour, 22);
    EXPECT_EQ(e.endHour, 6);
    EXPECT_EQ(e.intervalSecs, 7200);
    EXPECT_TRUE(e.blank);
}

TEST_F(DashboardDisplayTest, ClearReachesTheDisplay)
{
    EXPECT_EQ(result(frame.handleClear(httpRequest("POST", "/"))).status, 200);
    EXPECT_TRUE(waitFor([&] { return clears.size() == 1; }));
}

TEST_F(DashboardDisplayTest, InvalidRequestsNeverReachTheDisplay)
{
    EXPECT_EQ(result(frame.handleControl(httpRequest("POST", "/", {{"action", "pause"}}))).status, 400);
    EXPECT_EQ(result(frame.handleUpdateInterval(httpRequest("POST", "/", {{"interval_seconds", 60}}))).status, 400);
    EXPECT_EQ(result(frame.handleUpdateNightMode(httpRequest("POST", "/",
        {{"enabled", true}, {"start_hour", 3}, {"end_hour", 3}, {"interval_seconds", 3600}}))).status, 400);
    EXPECT_EQ(result(slideshowImages.handleUpdateOrder(httpRequest("POST", "/", {{"mode", "random"}}))).status, 400);
    EXPECT_EQ(result(slideshowImages.handleUpdateOrder(httpRequest("POST", "/",
        {{"mode", "custom"}, {"custom_order", {1, "2"}}}))).status, 400);
    EXPECT_EQ(frame.handleSkip(httpRequest("POST", "/"))->statusCode, 200);
    ASSERT_TRUE(waitFor([&] { return skips.size() == 1; }));
    EXPECT_EQ(active.size() + intervals.size() + nightModes.size() + orders.size(), 0u);
}

TEST_F(DashboardDisplayTest, StatusAggregatesTheDisplayState)
{
    settings.setDisplayInterval(1200);
    settings.setSlideshowActive(false);
    settings.setNightMode(true, 22, 6, 7200, false);
    repo.save({7, "alice", "/x.jpg", 0});

    const auto [status, body] = result(frame.handleStatus(httpRequest("GET", "/")));
    ASSERT_EQ(status, 200);
    const auto& d = body["data"];
    EXPECT_EQ(d["active"], false);
    EXPECT_EQ(d["interval_seconds"], 1200);
    EXPECT_EQ(d["image_count"], 1);
    EXPECT_EQ(d["seconds_until_next"], -1);
    EXPECT_EQ(d["night_mode"]["enabled"], true);
    EXPECT_EQ(d["night_mode"]["start_hour"], 22);
    EXPECT_EQ(d["night_mode"]["end_hour"], 6);
    EXPECT_EQ(d["night_mode"]["interval_seconds"], 7200);
    EXPECT_EQ(d["night_mode"]["blank"], false);
}

TEST_F(DashboardDisplayTest, DisplayStatsDeriveWearFromRefreshCount)
{
    metrics.set("epd_refresh_total", 250'000);
    const auto [status, body] = result(frame.handleDisplayStats(httpRequest("GET", "/")));
    ASSERT_EQ(status, 200);
    EXPECT_EQ(body["data"]["epd_refresh_total"], 250'000);
    EXPECT_DOUBLE_EQ(body["data"]["wear_percent"].get<double>(), 25.0);
    EXPECT_EQ(body["data"]["health"], "ok");
}

TEST_F(DashboardDisplayTest, UnreachableDisplayIsReportedNotSwallowed)
{
    server.stop();
    const auto [status, body] = result(frame.handleSkip(httpRequest("POST", "/")));
    EXPECT_EQ(status, 500);
    EXPECT_EQ(body["success"], false);
}

TEST_F(DashboardDisplayTest, PlaybackOrderArrivesAndSwitchingModeKeepsTheCustomOrder)
{
    EXPECT_EQ(result(slideshowImages.handleUpdateOrder(httpRequest("POST", "/",
        {{"mode", "custom"}, {"custom_order", {3, 1, 3, 2}}}))).status, 200);
    ASSERT_TRUE(waitFor([&] { return orders.size() == 1; }));
    EXPECT_EQ(orders.get()[0].mode, PlaybackMode::Custom);
    EXPECT_EQ(orders.get()[0].customOrder, (std::vector<int64_t>{3, 1, 2}));

    settings.setPlaybackOrder(PlaybackMode::Custom, {3, 1, 2});
    EXPECT_EQ(result(slideshowImages.handleUpdateOrder(httpRequest("POST", "/", {{"mode", "shuffle"}}))).status, 200);
    ASSERT_TRUE(waitFor([&] { return orders.size() == 2; }));
    EXPECT_EQ(orders.get()[1].mode, PlaybackMode::Shuffle);
    EXPECT_EQ(orders.get()[1].customOrder, (std::vector<int64_t>{3, 1, 2}));
}

TEST_F(DashboardDisplayTest, SlideshowImagesListInPlayOrderWithTheirFiles)
{
    const auto first = picture(1, 300);
    picture(2, 100);
    picture(3, 200);
    settings.setPlaybackOrder(PlaybackMode::Custom, {3});

    const auto [status, body] = result(slideshowImages.handleList(httpRequest("GET", "/")));
    ASSERT_EQ(status, 200);
    const auto& d = body["data"];
    EXPECT_EQ(d["mode"], "custom");
    ASSERT_EQ(d["images"].size(), 3u);
    EXPECT_EQ(d["images"][0]["id"], 3);
    EXPECT_EQ(d["images"][1]["id"], 2);
    EXPECT_EQ(d["images"][2]["id"], 1);
    const auto& img = d["images"][2];
    EXPECT_EQ(img["position"], 3);
    EXPECT_EQ(img["sender"], "sender1");
    EXPECT_EQ(img["received_at"], 300);
    EXPECT_EQ(img["expires_at"], 5001);
    EXPECT_GT(img["width"].get<int>(), 0);
    EXPECT_GT(img["height"].get<int>(), 0);
    EXPECT_EQ(img["size_bytes"], static_cast<int64_t>(std::filesystem::file_size(first)));

    const auto file = slideshowImages.handleFile(httpRequest("GET", "/"), {{"id", "1"}});
    EXPECT_EQ(file->statusCode, 200);
    EXPECT_EQ(file->headers["Content-Type"], "image/jpeg");
    EXPECT_EQ(file->body.size(), std::filesystem::file_size(first));
    EXPECT_EQ(slideshowImages.handleFile(httpRequest("GET", "/"), {{"id", "99"}})->statusCode, 404);
    EXPECT_EQ(slideshowImages.handleFile(httpRequest("GET", "/"), {{"id", "1; rm"}})->statusCode, 400);
}

TEST_F(DashboardDisplayTest, StatusNamesThePictureOnThePanel)
{
    const auto now = [&] { return result(frame.handleStatus(httpRequest("GET", "/"))).body["data"]["now_showing"]; };
    EXPECT_EQ(now()["state"], "empty");

    const auto path = picture(7, 100);
    ASSERT_TRUE(display.displayBuffer(std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF), path.string(), {},
                                      RefreshPolicy::Immediate));
    EXPECT_EQ(now()["state"], "image");
    EXPECT_EQ(now()["image_id"], 7);
    EXPECT_EQ(now()["sender"], "sender7");
    EXPECT_EQ(now()["seq"], 1);

    cfg.display.minRefreshSecs = 0;
    display.clear();
    EXPECT_EQ(now()["state"], "cleared");
    EXPECT_EQ(now()["image_id"], 0);
    EXPECT_EQ(now()["seq"], 2);
}

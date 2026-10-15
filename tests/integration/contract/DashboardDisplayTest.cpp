#include "dashboard/FrameHandlers.hpp"
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
    Recorder<Topic::UPDATE_IMAGE_QUALITY> qualities{bus};
    Recorder<Topic::CLEAR_DISPLAY> clears{bus};

    NngRepServer server{url, [this](const IpcMessage& m) { return displayIpc.handle(m); }};
    IpcClient ipc{url};
    FrameHandlers frame{ipc, cfg};
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
        {{"enabled", true}, {"start_hour", 22}, {"end_hour", 6}, {"interval_seconds", 7200}})));
    EXPECT_EQ(r.status, 200);
    ASSERT_TRUE(waitFor([&] { return nightModes.size() == 1; }));
    const auto e = nightModes.get()[0];
    EXPECT_TRUE(e.enabled);
    EXPECT_EQ(e.startHour, 22);
    EXPECT_EQ(e.endHour, 6);
    EXPECT_EQ(e.intervalSecs, 7200);
}

TEST_F(DashboardDisplayTest, ImageQualityArrivesAndReadsBack)
{
    const auto r = result(frame.handleUpdateImageQuality(httpRequest("POST", "/", {{"quality", "gray4"}})));
    EXPECT_EQ(r.status, 200);
    ASSERT_TRUE(waitFor([&] { return qualities.size() == 1; }));
    EXPECT_EQ(qualities.get()[0].quality, ImageQuality::Gray4);

    EXPECT_EQ(result(frame.handleImageQuality(httpRequest("GET", "/"))).body["data"]["quality"], "mono");
    settings.setImageQuality(ImageQuality::Gray4);
    EXPECT_EQ(result(frame.handleImageQuality(httpRequest("GET", "/"))).body["data"]["quality"], "gray4");
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
    EXPECT_EQ(result(frame.handleUpdateImageQuality(httpRequest("POST", "/", {{"quality", "gray16"}}))).status, 400);
    EXPECT_EQ(frame.handleSkip(httpRequest("POST", "/"))->statusCode, 200);
    ASSERT_TRUE(waitFor([&] { return skips.size() == 1; }));
    EXPECT_EQ(active.size() + intervals.size() + nightModes.size() + qualities.size(), 0u);
}

TEST_F(DashboardDisplayTest, StatusAggregatesTheDisplayState)
{
    settings.setDisplayInterval(1200);
    settings.setSlideshowActive(false);
    settings.setNightMode(true, 22, 6, 7200);
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

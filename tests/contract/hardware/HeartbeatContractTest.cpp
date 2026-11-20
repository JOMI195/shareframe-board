#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "repository/ImageRepository.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/Golden.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "task/Heartbeat.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <regex>

using namespace std::chrono_literals;

// The heartbeat forwards the event log lines shareframe-event prints since its cursor.
TEST(HeartbeatContract, EventsAreParsedFromTheEventLog)
{
    PathShim shims;
    shims.add("shareframe-event", "cat '" + golden::hardware("event-since.txt").string() + "'");
    shims.add("shareframe-sysinfo", "exit 0");
    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    cfg.baseUrl = backend.baseUrl();
    cfg.frameId = "ABCD-EFGH-IJKL-MNOP";
    cfg.heartbeat.intervalSecs = 3600;
    cfg.heartbeat.firstContactRetrySecs = 3600;
    cfg.heartbeat.eventCursorPath = (db.dir() / "event-cursor").string();
    cfg.heartbeat.backendOkPath = (db.dir() / "backend-ok").string();
    storeValidToken(db.get());
    constexpr auto kHeartbeat = "/api/frames/frame-hearbeat/";
    backend.on(std::string("POST ") + kHeartbeat, 200);
    TokenRepository tokens{db.get()};
    ImageRepository images{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    EventBus bus;
    auto sock = [&](const char* n) { return "ipc://" + (db.dir() / n).string(); };
    IpcClient wsIpc{sock("ws.sock")}, displayIpc{sock("display.sock")}, dashboardIpc{sock("dash.sock")},
        updateIpc{sock("update.sock")};
    Heartbeat heartbeat{bus, cfg, http, auth, wsIpc, displayIpc, dashboardIpc, updateIpc, images};

    heartbeat.start();
    ASSERT_TRUE(waitFor([&] { return !backend.requests(kHeartbeat).empty(); }, 5000ms));
    ASSERT_TRUE(waitFor([&] { return std::filesystem::exists(cfg.heartbeat.backendOkPath); }));
    heartbeat.stop();
    const auto events = nlohmann::json::parse(backend.requests(kHeartbeat)[0].body).at("events");

    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].at("seq"), 1);
    EXPECT_EQ(events[0].at("time"), "2026-01-02T03:04:05");
    EXPECT_EQ(events[0].at("source"), "boot");
    EXPECT_EQ(events[0].at("message"), "clean boot 12");
    EXPECT_EQ(events[1].at("source"), "wifi");
    EXPECT_EQ(events[1].at("message"), "ap fallback (no address from home)");
    EXPECT_EQ(events[2].at("seq"), 3);

    // wifi-mode-manager reads the mark as unix seconds on one line: until when the contact counts.
    std::ifstream f(cfg.heartbeat.backendOkPath);
    const std::string mark((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(std::regex_match(mark, std::regex("[0-9]+\n"))) << mark;
}

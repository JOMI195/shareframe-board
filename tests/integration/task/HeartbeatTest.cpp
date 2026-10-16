#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "task/Heartbeat.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace std::chrono_literals;

namespace
{
constexpr auto kHeartbeat = "/api/frames/frame-hearbeat/";

class HeartbeatTest : public ::testing::Test
{
protected:
    HeartbeatTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.frameId = "ABCD-EFGH-IJKL-MNOP";
        cfg.heartbeat.intervalSecs = 3600;
        cfg.heartbeat.eventCursorPath = (db.dir() / "event-cursor").string();
        cfg.heartbeat.firstContactRetrySecs = 3600;
        cfg.heartbeat.backendOkPath = (db.dir() / "backend-ok").string();
        storeValidToken(db.get());
        backend.on(std::string("POST ") + kHeartbeat, 200);
        wsHealth.start();
        updateHealth.start();
    }

    ~HeartbeatTest() override { heartbeat.stop(); }

    nlohmann::json sentPayload()
    {
        heartbeat.start();
        EXPECT_TRUE(waitFor([&] { return !backend.requests(kHeartbeat).empty(); }, 5000ms));
        const auto requests = backend.requests(kHeartbeat);
        return requests.empty() ? nlohmann::json{} : nlohmann::json::parse(requests[0].body);
    }

    std::string sock(const std::string& name) const { return "ipc://" + (db.dir() / name).string(); }

    PathShim shims;
    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    EventBus bus;
    NngRepServer wsHealth{sock("ws.sock"), health::respond};
    NngRepServer updateHealth{sock("update.sock"), health::respond};
    IpcClient wsIpc{sock("ws.sock")};
    IpcClient displayIpc{sock("display.sock")};
    IpcClient dashboardIpc{sock("dashboard.sock")};
    IpcClient updateIpc{sock("update.sock")};
    Heartbeat heartbeat{bus, cfg, http, auth, wsIpc, displayIpc, dashboardIpc, updateIpc};
};
}

TEST_F(HeartbeatTest, ReportsServiceHealthAndTypedSystemMetrics)
{
    shims.add("shareframe-sysinfo", "printf 'uptime_seconds=12\\ncpu_temp_celsius=41.5\\nhealth_state=ok\\n"
                                    "not_reported=1\\ngarbage\\n'");
    const auto p = sentPayload();

    EXPECT_EQ(p["serial_number"], "ABCD-EFGH-IJKL-MNOP");
    EXPECT_EQ(p["websocket_running"], true);
    EXPECT_EQ(p["display_running"], false);
    EXPECT_EQ(p["dashboard_running"], false);
    EXPECT_EQ(p["update_running"], true);
    EXPECT_EQ(p["uptime_seconds"], 12);
    EXPECT_DOUBLE_EQ(p["cpu_temp_celsius"].get<double>(), 41.5);
    EXPECT_EQ(p["health_state"], "ok");
    EXPECT_FALSE(p.contains("not_reported"));
    EXPECT_EQ(backend.requests(kHeartbeat)[0].headers.at("Authorization"), "Frame-Access-Token test-token");
}

TEST_F(HeartbeatTest, FailingSysinfoStillSendsTheHeartbeat)
{
    shims.add("shareframe-sysinfo", "exit 1");
    const auto p = sentPayload();
    EXPECT_EQ(p["websocket_running"], true);
    EXPECT_FALSE(p.contains("uptime_seconds"));
}

TEST_F(HeartbeatTest, RejectedHeartbeatDropsTheTokenForRefetch)
{
    shims.add("shareframe-sysinfo", "exit 0");
    backend.on(std::string("POST ") + kHeartbeat, 401);
    sentPayload();
    EXPECT_TRUE(waitFor([&] { return !tokens.get().has_value(); }));
}

TEST_F(HeartbeatTest, ForwardsFaultCountersAndPerProcessMemory)
{
    shims.add("shareframe-sysinfo", "printf 'crash_count=2\\nwlan_signal_dbm=-61\\nwifi_ssid=Home\\n"
                                    "proc_display_heap_bytes=1048576\\nproc_update_threads=3\\n"
                                    "data_written_kb=8589934592\\nsd_card=SC32G mfr=0x000003 oem=0x5344 date=05/2023\\n"
                                    "safe_mode=display-crash\\n'");
    const auto p = sentPayload();

    EXPECT_EQ(p["crash_count"], 2);
    EXPECT_EQ(p["data_written_kb"], 8589934592);
    EXPECT_EQ(p["sd_card"], "SC32G mfr=0x000003 oem=0x5344 date=05/2023");
    EXPECT_EQ(p["safe_mode"], "display-crash");
    EXPECT_EQ(p["wlan_signal_dbm"], -61);
    EXPECT_EQ(p["proc_display_heap_bytes"], 1048576);
    EXPECT_EQ(p["proc_update_threads"], 3);
    EXPECT_FALSE(p.contains("wifi_ssid"));
}

TEST_F(HeartbeatTest, IncludesDisplayStatsWhenDisplayIsUp)
{
    shims.add("shareframe-sysinfo", "exit 0");
    NngRepServer display{sock("display.sock"), [](const IpcMessage& msg) -> nlohmann::json {
        if (msg.type == IpcMessageType::GetDisplayStats)
            return {{"epd_refresh_total", 42}, {"health", "ok"}};
        return health::respond(msg);
    }};
    display.start();
    const auto p = sentPayload();

    EXPECT_EQ(p["display_running"], true);
    EXPECT_EQ(p["display_epd_refresh_total"], 42);
    EXPECT_EQ(p["display_health"], "ok");
}

TEST_F(HeartbeatTest, SendsEventsSinceCursorAndAdvancesIt)
{
    shims.add("shareframe-sysinfo", "exit 0");
    shims.add("shareframe-event", "[ \"$2\" = 0 ] || exit 0\n"
                                  "printf '7 2026-10-16T10:00:00 [crash] shareframe-display signal 11\\n"
                                  "8 2026-10-16T10:01:00 [boot] version=1.2.0 unclean=1\\ngarbage\\n'\n"
                                  "printf '%s\\n' '9 2026-10-16T10:02:00 [update] failed (\"bad\" sig)'");
    const auto p = sentPayload();

    ASSERT_EQ(p["events"].size(), 3u);
    EXPECT_EQ(p["events"][0]["seq"], 7);
    EXPECT_EQ(p["events"][0]["source"], "crash");
    EXPECT_EQ(p["events"][0]["message"], "shareframe-display signal 11");
    EXPECT_EQ(p["events"][1]["time"], "2026-10-16T10:01:00");
    EXPECT_EQ(p["events"][2]["message"], "failed ( bad  sig)");
    EXPECT_TRUE(waitFor([&] {
        int64_t cursor = 0;
        std::ifstream(cfg.heartbeat.eventCursorPath) >> cursor;
        return cursor == 9;
    }));
}

TEST_F(HeartbeatTest, KeepsEventCursorWhenHeartbeatFails)
{
    shims.add("shareframe-sysinfo", "exit 0");
    shims.add("shareframe-event", "printf '3 2026-10-16T10:00:00 [wifi] connected\\n'");
    backend.on(std::string("POST ") + kHeartbeat, 500);
    const auto p = sentPayload();

    EXPECT_EQ(p["events"].size(), 1u);
    EXPECT_FALSE(std::filesystem::exists(cfg.heartbeat.eventCursorPath));
    EXPECT_FALSE(std::filesystem::exists(cfg.heartbeat.backendOkPath));
}

TEST_F(HeartbeatTest, AcknowledgedHeartbeatMarksTheBackendReached)
{
    shims.add("shareframe-sysinfo", "exit 0");
    sentPayload();
    EXPECT_TRUE(waitFor([&] { return std::filesystem::exists(cfg.heartbeat.backendOkPath); }));
}

TEST_F(HeartbeatTest, RetriesFastOnlyUntilTheFirstAcknowledgedHeartbeat)
{
    cfg.heartbeat.firstContactRetrySecs = 1;
    shims.add("shareframe-sysinfo", "exit 0");
    backend.on(std::string("POST ") + kHeartbeat, 500);
    heartbeat.start();
    ASSERT_TRUE(waitFor([&] { return backend.requests(kHeartbeat).size() == 1; }, 5000ms));

    backend.on(std::string("POST ") + kHeartbeat, 200);
    ASSERT_TRUE(waitFor([&] { return std::filesystem::exists(cfg.heartbeat.backendOkPath); }, 5000ms));
    const auto sent = backend.requests(kHeartbeat).size();
    std::this_thread::sleep_for(1500ms);
    EXPECT_EQ(backend.requests(kHeartbeat).size(), sent);
}

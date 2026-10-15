#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "task/Heartbeat.hpp"
#include <gtest/gtest.h>

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

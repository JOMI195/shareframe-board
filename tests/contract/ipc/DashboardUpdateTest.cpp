#include "auth/AuthTokenManager.hpp"
#include "dashboard/UpdateHandlers.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/Http.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "update/UpdateIpc.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

// Dashboard HTTP handler → nng → update service → Shareframe server, for every update route.
namespace
{
constexpr auto kLatest = "/api/frame-updates/latest/";

class DashboardUpdateTest : public ::testing::Test
{
protected:
    DashboardUpdateTest()
    {
        cfg.baseUrl = backend.baseUrl();
        storeValidToken(db.get());
        server.start();
    }

    ~DashboardUpdateTest() override { server.stop(); }

    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    IpcClient displayIpc{"ipc://" + (db.dir() / "display.sock").string()};
    UpdateManager updates{cfg, http, auth, displayIpc};
    UpdateIpc updateIpc{updates};
    std::string url = "ipc://" + (db.dir() / "update.sock").string();
    NngRepServer server{url, [this](const IpcMessage& m) { return updateIpc.handle(m); }, 2};
    IpcClient ipc{url};
    UpdateHandlers handlers{ipc};
};
}

TEST_F(DashboardUpdateTest, LatestReleaseIsFetchedWithTheFrameToken)
{
    backend.on(std::string("GET ") + kLatest, 200, R"({"version":"9.9.9","criticality":"Low"})");

    const auto [status, body] = result(handlers.handleLatest(httpRequest("GET", "/")));
    ASSERT_EQ(status, 200);
    EXPECT_EQ(body["data"]["version"], "9.9.9");
    ASSERT_EQ(backend.requests(kLatest).size(), 1u);
    EXPECT_EQ(backend.requests(kLatest)[0].headers.at("Authorization"), "Frame-Access-Token test-token");
}

TEST_F(DashboardUpdateTest, LatestReleaseCarriesTheNewerVerdict)
{
    cfg.version = "1.2.0";
    backend.on(std::string("GET ") + kLatest, 200, R"({"version":"1.10.0"})");
    EXPECT_EQ(result(handlers.handleLatest(httpRequest("GET", "/"))).body["data"]["is_newer"], true);

    backend.on(std::string("GET ") + kLatest, 200, R"({"version":"1.2.0"})");
    EXPECT_EQ(result(handlers.handleLatest(httpRequest("GET", "/"))).body["data"]["is_newer"], false);
}

TEST_F(DashboardUpdateTest, NoReleaseOnTheServerIsNotAnError)
{
    const auto [status, body] = result(handlers.handleLatest(httpRequest("GET", "/")));
    EXPECT_EQ(status, 200);
    EXPECT_TRUE(body["data"].is_null());
}

TEST_F(DashboardUpdateTest, ServerFailureBecomesBadGateway)
{
    backend.on(std::string("GET ") + kLatest, 500);
    EXPECT_EQ(result(handlers.handleLatest(httpRequest("GET", "/"))).status, 502);
}

TEST_F(DashboardUpdateTest, PerformRefusesWhenNothingIsNewer)
{
    backend.on(std::string("GET ") + kLatest, 200, R"({"version":"not-a-release"})");
    const auto [status, body] = result(handlers.handlePerformUpdate(httpRequest("POST", "/")));
    EXPECT_EQ(status, 409);
    EXPECT_EQ(body["message"], "No newer version available");
    EXPECT_EQ(result(handlers.handleStatus(httpRequest("GET", "/"))).body["data"]["phase"], "idle");
}

TEST_F(DashboardUpdateTest, MalformedReleaseIsRefusedAndTheServiceSurvives)
{
    backend.on(std::string("GET ") + kLatest, 200, R"({"version":2,"criticality":["Critical"]})");
    const auto [status, body] = result(handlers.handlePerformUpdate(httpRequest("POST", "/")));
    EXPECT_EQ(status, 409);
    EXPECT_EQ(body["message"], "Failed to fetch latest release from server");
    EXPECT_EQ(result(handlers.handleStatus(httpRequest("GET", "/"))).status, 200);
}

TEST_F(DashboardUpdateTest, StatusAndHistoryAreServed)
{
    const auto status = result(handlers.handleStatus(httpRequest("GET", "/")));
    ASSERT_EQ(status.status, 200);
    EXPECT_EQ(status.body["data"]["phase"], "idle");
    EXPECT_TRUE(status.body["data"].contains("current_version"));

    const auto history = result(handlers.handleHistory(httpRequest("GET", "/")));
    ASSERT_EQ(history.status, 200);
    EXPECT_TRUE(history.body["data"]["history"].is_array());
}

TEST_F(DashboardUpdateTest, UnreachableUpdateServiceIsReported)
{
    server.stop();
    EXPECT_EQ(result(handlers.handleStatus(httpRequest("GET", "/"))).status, 503);
}

// The dashboard polls the status every few seconds; a slow update server must not stall it.
TEST_F(DashboardUpdateTest, StatusAnswersWhileAnInstallWaitsOnTheUpdateServer)
{
    using namespace std::chrono_literals;
    backend.during(std::string("GET ") + kLatest, [] { std::this_thread::sleep_for(3s); });
    backend.on(std::string("GET ") + kLatest, 200, R"({"version":"0.0.1"})");
    IpcClient installer{url};
    std::thread install([&] { installer.sendAndReceive({IpcMessageType::UpdatePerform, {}}, 10000ms); });
    std::this_thread::sleep_for(500ms);

    const auto start = std::chrono::steady_clock::now();
    const auto [status, body] = result(handlers.handleStatus(httpRequest("GET", "/")));
    const auto took = std::chrono::steady_clock::now() - start;
    install.join();

    EXPECT_EQ(status, 200) << body.dump();
    EXPECT_LT(took, 1500ms);
}

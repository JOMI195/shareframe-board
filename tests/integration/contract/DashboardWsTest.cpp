#include "dashboard/WifiHandlers.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/WsIpc.hpp"
#include "support/TestDb.hpp"
#include <ctime>
#include <fstream>
#include <gtest/gtest.h>

// Dashboard connection mode → nng → websocket service's server link.
namespace
{
class DashboardWsTest : public ::testing::Test
{
protected:
    DashboardWsTest() { server.start(); }
    ~DashboardWsTest() override { server.stop(); }

    TestDb db;
    std::string url = "ipc://" + (db.dir() / "ws.sock").string();
    SettingsRepository settings{db.get()};
    int64_t now = std::time(nullptr) - 7200;
    ServerContact contact{settings, [this] { return now; }};
    WsIpc wsIpc{contact};
    NngRepServer server{url, [this](const IpcMessage& m) { return wsIpc.handle(m); }};
    IpcClient ipc{url};
    WifiManager wifi{(db.dir() / "wifi-mode.json").string()};
    WifiHandlers handlers{wifi, ipc};
};
}

TEST_F(DashboardWsTest, ConnectionModeCarriesTheServerLink)
{
    contact.onOpen();
    now += 3600;
    contact.onClose();

    const auto mode = handlers.connectionMode();
    EXPECT_EQ(mode["server_connected"], false);
    EXPECT_EQ(mode["server_last_contact_at"], now);
    EXPECT_NEAR(mode["server_offline_secs"].get<int64_t>(), 3600, 5);
}

TEST_F(DashboardWsTest, ConnectionModeHidesSavedNetworks)
{
    std::ofstream(db.dir() / "wifi-mode.json") << R"({"mode":"ap","known_ssids":["Jomi","HomeNet"]})";

    const auto mode = handlers.connectionMode();
    EXPECT_EQ(mode["mode"], "ap");
    EXPECT_FALSE(mode.contains("known_ssids"));
}

TEST_F(DashboardWsTest, StillAnswersHealthProbes)
{
    EXPECT_TRUE(health::isRunning(ipc));
}

TEST_F(DashboardWsTest, SilentWebsocketServiceIsReportedAsUnknown)
{
    server.stop();
    const auto mode = handlers.connectionMode();
    EXPECT_TRUE(mode["server_connected"].is_null());
    EXPECT_EQ(mode["server_offline_secs"], 0);
}

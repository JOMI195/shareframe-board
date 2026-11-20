#include "dashboard/WifiHandlers.hpp"
#include "support/Http.hpp"
#include "support/PathShim.hpp"
#include "support/TempDir.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class WifiHandlersTest : public ::testing::Test
{
protected:
    void mode(const std::string& json) const { std::ofstream(dir.path() / "wifi-mode.json") << json; }
    bool retryRequested() const { return std::filesystem::exists(dir.path() / "wifi-retry-requested"); }

    PathShim shims;
    TempDir dir;
    IpcClient ws{"ipc://" + (dir.path() / "silent.sock").string()};
    WifiManager wifi{(dir.path() / "wifi-mode.json").string()};
    WifiHandlers handlers{wifi, ws};
};
}

TEST_F(WifiHandlersTest, ApplyOnTheApAsksTheDaemonToTryTheSavedNetworks)
{
    mode(R"({"mode":"ap"})");
    EXPECT_EQ(result(handlers.handleApply(httpRequest("POST", "/api/connection/apply"))).status, 200);
    EXPECT_TRUE(retryRequested());
}

TEST_F(WifiHandlersTest, ApplyOutsideTheApIsRefused)
{
    mode(R"({"mode":"connected"})");
    EXPECT_EQ(result(handlers.handleApply(httpRequest("POST", "/api/connection/apply"))).status, 409);
    EXPECT_FALSE(retryRequested());
}

TEST_F(WifiHandlersTest, ModeSaysHowLongAgoTheLastAttemptFailed)
{
    mode(R"({"mode":"ap","last_failed_at":)" + std::to_string(std::time(nullptr) - 120) + "}");
    auto m = handlers.connectionMode();
    EXPECT_FALSE(m.contains("last_failed_at"));
    EXPECT_NEAR(m["last_failed_secs_ago"].get<int64_t>(), 120, 5);

    mode(R"({"mode":"connected","last_failed_at":0})");
    m = handlers.connectionMode();
    EXPECT_TRUE(m["last_failed_secs_ago"].is_null());
}

TEST_F(WifiHandlersTest, AnOpenNetworkIsSavedWithoutAPassword)
{
    const auto calls = dir.path() / "calls";
    shims.add("shareframe-wifi", "printf '%s|' \"$@\" >> " + calls.string());
    const auto r = result(handlers.handleConnect(httpRequest("POST", "/api/connection/connect",
                                                             {{"ssid", "Cafe"}, {"password", ""}})));
    EXPECT_EQ(r.status, 200);
    std::ifstream f(calls);
    std::string args((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_EQ(args, "add|Cafe||");
}

TEST_F(WifiHandlersTest, AShortPasswordIsRefusedBeforeAnythingIsSaved)
{
    shims.add("shareframe-wifi", "exit 1");
    const auto r = result(handlers.handleConnect(httpRequest("POST", "/api/connection/connect",
                                                             {{"ssid", "Cafe"}, {"password", "short"}})));
    EXPECT_EQ(r.status, 400);
}

// Things the owner can fix answer 409; only the board failing answers 500.
TEST_F(WifiHandlersTest, ForgettingTheActiveNetworkIsAConflict)
{
    shims.add("shareframe-wifi", "[ \"$1\" = status ] && printf 'wpa_state=COMPLETED\\nssid=home\\n'; exit 0");
    const auto r = result(handlers.handleForget(httpRequest("POST", "/api/connection/forget", {{"ssid", "home"}})));
    EXPECT_EQ(r.status, 409);
    EXPECT_EQ(r.body["message"], "Active network");
}

TEST_F(WifiHandlersTest, AScanWithoutResultsSaysSo)
{
    const auto r = result(handlers.handleScan(httpRequest("GET", "/api/connection/scan")));
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(r.body["data"]["networks"], nlohmann::json::array());
    EXPECT_TRUE(r.body["data"]["age_secs"].is_null());
}

#include "dashboard/WifiHandlers.hpp"
#include "support/Http.hpp"
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

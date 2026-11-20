#include "auth/AuthTokenManager.hpp"
#include "dashboard/SystemHandlers.hpp"
#include "dashboard/WifiManager.hpp"
#include "net/DiagnosticsUploader.hpp"
#include "support/Http.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class SystemHandlersTest : public ::testing::Test
{
protected:
    SystemHandlersTest()
    {
        cfg.version = "1.4.0";
        cfg.log.logPath = db.dir().path().string();
        cfg.log.systemLogPath = (db.dir() / "messages").string();
        std::ofstream(cfg.log.systemLogPath) << "line 1\nline 2\nline 3\n";
        shims.add("shareframe-sysinfo", "printf 'serial=000123ab\\nuptime_seconds=60\\n'");
        shims.add("shareframe-wifi", "printf 'ssid=my home net\\nwpa_state=COMPLETED\\n'");
    }

    HttpResult logs(const std::unordered_map<std::string, std::string>& query)
    {
        return result(system.handleLogs(httpRequest("GET", "/api/system/logs"), query));
    }

    PathShim shims;
    TestDb db;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{1, 1};
    AuthTokenManager auth{cfg, tokens, http};
    DiagnosticsUploader diagnostics{cfg, http, auth};
    WifiManager wifi{(db.dir() / "wifi-mode.json").string()};
    IpcClient ipc{"ipc://" + (db.dir() / "none.sock").string()};
    SystemHandlers system{cfg, wifi, diagnostics, ipc};
};
}

TEST_F(SystemHandlersTest, InfoCombinesTheBoardMetricsTheBuildAndTheNetwork)
{
    const auto data = result(system.handleInfo(httpRequest("GET", "/api/system/info"))).body["data"];
    EXPECT_EQ(data["serial"], "000123ab");
    EXPECT_EQ(data["uptime_seconds"], 60);
    EXPECT_EQ(data["version"], "1.4.0");
    EXPECT_EQ(data["wlan_ssid"], "my home net");
}

// Every open tab polls this page; the board tool runs once for all of them.
TEST_F(SystemHandlersTest, InfoAskedTwiceInARowRunsTheBoardToolOnce)
{
    const auto calls = db.dir() / "sysinfo-calls";
    shims.add("shareframe-sysinfo", "echo x >> " + calls.string() + "; printf 'serial=000123ab\\n'");
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(result(system.handleInfo(httpRequest("GET", "/api/system/info"))).body["data"]["serial"], "000123ab");

    std::ifstream f(calls);
    int lines = 0;
    for (std::string line; std::getline(f, line);)
        ++lines;
    EXPECT_EQ(lines, 1);
}

// The dashboard's device page must not go blank when one board tool misbehaves.
TEST_F(SystemHandlersTest, InfoStillAnswersWhenTheBoardMetricsFail)
{
    shims.add("shareframe-sysinfo", "echo broken >&2; exit 1");
    const auto r = result(system.handleInfo(httpRequest("GET", "/api/system/info")));
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(r.body["data"]["version"], "1.4.0");
}

TEST_F(SystemHandlersTest, LogsShowTheLastLinesOfTheChosenLog)
{
    const auto r = logs({{"service_name", "system"}, {"lines", "2"}});
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(r.body["data"]["logs"], "line 2\nline 3\n");
}

TEST_F(SystemHandlersTest, TheLineCountIsClampedAndGarbageFallsBackToTheDefault)
{
    EXPECT_EQ(logs({{"service_name", "system"}, {"lines", "999999"}}).body["data"]["lines"], 10000);
    EXPECT_EQ(logs({{"service_name", "system"}, {"lines", "-5"}}).body["data"]["lines"], 100);
    EXPECT_EQ(logs({{"service_name", "system"}, {"lines", "abc"}}).body["data"]["lines"], 100);
}

// The name picks a file: only the known ids, nothing that walks the file system.
TEST_F(SystemHandlersTest, OnlyKnownLogsCanBeRead)
{
    for (const auto* name : {"shadow", "../../etc/shadow", ""})
        EXPECT_EQ(logs({{"service_name", name}}).status, 400) << name;
    EXPECT_EQ(logs({}).status, 400);
}

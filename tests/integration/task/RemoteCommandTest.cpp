#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "task/RemoteCommand.hpp"
#include <atomic>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

using namespace std::chrono_literals;

namespace
{
constexpr auto kDiagnostics = "/api/frames/frame-diagnostics/";

class RemoteCommandTest : public ::testing::Test
{
protected:
    RemoteCommandTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.diagnostics.pendingPath = (db.dir() / "pre-reboot-diag.gz").string();
        storeValidToken(db.get());
        backend.on(std::string("POST ") + kDiagnostics, 201);

        const auto log = calls.string();
        for (const auto* tool : {"reboot", "shutdown", "s6-svc", "shareframe-factory-reset", "shareframe-event",
                                 "shareframe-safe-mode"})
            shims.add(tool, "echo \"" + std::string(tool) + " $*\" >> '" + log + "'");
        shims.add("shareframe-remote-gate", "echo \"gate $*\" >> '" + log + "'");
        shims.add("shareframe-diag", "[ \"$1\" = --out ] && printf pkg > \"$2\" || printf pkg");

        bus.subscribe<Topic::WS_SEND>([this](const nlohmann::json& msg) {
            std::lock_guard lk(sentMtx);
            sent.push_back(msg);
        });
        display.start();
        update.start();
        commands.start();
        bus.start();
    }

    ~RemoteCommandTest() override
    {
        commands.stop();
        bus.stop();
    }

    void send(const std::string& command, const std::string& arg = "", int64_t id = 1, int64_t ttl = 600)
    {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        bus.publish<Topic::REMOTE_COMMAND>(
            {{"type", "command"}, {"id", id}, {"command", command}, {"arg", arg}, {"expires_at", now + ttl}});
    }

    std::vector<std::string> statuses()
    {
        std::lock_guard lk(sentMtx);
        std::vector<std::string> out;
        for (const auto& m : sent)
            out.push_back(m["status"].get<std::string>() + (m["reason"].get<std::string>().empty() ? "" : ":" + m["reason"].get<std::string>()));
        return out;
    }

    bool waitStatuses(const std::vector<std::string>& expected)
    {
        return waitFor([&] { return statuses() == expected; }, 5000ms);
    }

    std::string calledTools() const
    {
        std::ifstream in(calls);
        std::stringstream s;
        s << in.rdbuf();
        return s.str();
    }

    std::string sock(const std::string& name) const { return "ipc://" + (db.dir() / name).string(); }

    PathShim shims;
    FakeHttpServer backend;
    TestDb db;
    std::filesystem::path calls = db.dir() / "calls.log";
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    DiagnosticsUploader diagnostics{cfg, http, auth};
    EventBus bus;

    std::vector<std::string> shownStatusScreens()
    {
        std::lock_guard lk(sentMtx);
        return statusScreens;
    }

    std::atomic<bool> refreshing{false};
    std::string updatePhase = "idle";
    bool newerRelease = true;
    NngRepServer display{sock("display.sock"), [this](const IpcMessage& msg) -> nlohmann::json {
        if (msg.type == IpcMessageType::GetSlideshowActive)
            return {{"refreshing", refreshing.load()}};
        if (msg.type == IpcMessageType::ShowStatusScreen || msg.type == IpcMessageType::HideStatusScreen)
        {
            std::lock_guard lk(sentMtx);
            statusScreens.push_back(msg.type == IpcMessageType::HideStatusScreen ? "hide" : msg.data.value("screen", ""));
        }
        return health::respond(msg);
    }};
    NngRepServer update{sock("update.sock"), [this](const IpcMessage& msg) -> nlohmann::json {
        if (msg.type == IpcMessageType::UpdateStatus)
            return {{"ok", true}, {"status", {{"phase", updatePhase}}}};
        if (msg.type == IpcMessageType::UpdateCheckLatest)
            return {{"ok", true}, {"release", {{"version", "9.9.9"}, {"is_newer", newerRelease}}}};
        if (msg.type == IpcMessageType::UpdatePerform)
            return {{"ok", true}, {"error", ""}};
        return health::respond(msg);
    }};
    IpcClient displayIpc{sock("display.sock")};
    IpcClient updateIpc{sock("update.sock")};
    RemoteCommand commands{bus, displayIpc, updateIpc, diagnostics, {500ms, 50ms, 0ms}};

    std::mutex sentMtx;
    std::vector<nlohmann::json> sent;
    std::vector<std::string> statusScreens;
};
}

TEST_F(RemoteCommandTest, RestartsAnAllowedServiceThroughS6)
{
    send("restart_service", "display");
    ASSERT_TRUE(waitStatuses({"accepted", "done"}));
    EXPECT_NE(calledTools().find("s6-svc -r /run/service/shareframe-display"), std::string::npos);
    EXPECT_NE(calledTools().find("shareframe-event remote #1 restart_service display accepted"), std::string::npos);
}

TEST_F(RemoteCommandTest, RejectsCommandsOutsideTheAllowlist)
{
    send("shell", "rm -rf /", 1);
    send("restart_service", "dropbear", 2);
    send("reboot", "now", 3);
    ASSERT_TRUE(waitStatuses({"rejected:unknown command", "rejected:invalid argument", "rejected:invalid argument"}));
    EXPECT_EQ(calledTools().find("s6-svc"), std::string::npos);
    EXPECT_EQ(calledTools().find("reboot \n"), std::string::npos);
}

TEST_F(RemoteCommandTest, RejectsExpiredCommands)
{
    send("reboot", "", 1, -1);
    ASSERT_TRUE(waitStatuses({"rejected:expired"}));
}

TEST_F(RemoteCommandTest, RebootCapturesEvidenceAndMarksTheRateLimit)
{
    send("reboot");
    ASSERT_TRUE(waitFor([&] { return calledTools().find("reboot \n") != std::string::npos; }, 5000ms));
    EXPECT_EQ(statuses(), std::vector<std::string>{"accepted"});

    const auto tools = calledTools();
    EXPECT_LT(tools.find("gate check"), tools.find("gate mark"));
    EXPECT_EQ(tools.find("--no-rate-limit"), std::string::npos);
    EXPECT_LT(tools.find("gate mark"), tools.find("reboot \n"));
    EXPECT_EQ(shownStatusScreens(), std::vector<std::string>{"reboot"});
    const auto uploads = backend.requests(kDiagnostics);
    ASSERT_EQ(uploads.size(), 1u);
    EXPECT_EQ(uploads[0].uri, std::string(kDiagnostics) + "?trigger=pre-reboot&command_id=1");
}

TEST_F(RemoteCommandTest, ShutdownCapturesEvidenceAndLeavesNoStatusScreen)
{
    send("shutdown");
    ASSERT_TRUE(waitFor([&] { return calledTools().find("shutdown \n") != std::string::npos; }, 5000ms));
    EXPECT_EQ(statuses(), std::vector<std::string>{"accepted"});

    const auto tools = calledTools();
    EXPECT_LT(tools.find("gate check"), tools.find("gate mark"));
    EXPECT_LT(tools.find("gate mark"), tools.find("shutdown \n"));
    EXPECT_TRUE(shownStatusScreens().empty());
    const auto uploads = backend.requests(kDiagnostics);
    ASSERT_EQ(uploads.size(), 1u);
    EXPECT_EQ(uploads[0].uri, std::string(kDiagnostics) + "?trigger=pre-reboot&command_id=1");
}

TEST_F(RemoteCommandTest, NeverRebootsWhileAnUpdateRuns)
{
    updatePhase = "installing";
    send("reboot");
    ASSERT_TRUE(waitStatuses({"rejected:update installing"}));
    EXPECT_EQ(calledTools().find("reboot \n"), std::string::npos);
}

TEST_F(RemoteCommandTest, WaitsForTheDisplayRefreshThenGivesUp)
{
    refreshing = true;
    send("reboot");
    ASSERT_TRUE(waitStatuses({"rejected:display still refreshing"}));
    EXPECT_EQ(calledTools().find("reboot \n"), std::string::npos);
}

TEST_F(RemoteCommandTest, RebootsOnceTheRefreshFinishes)
{
    refreshing = true;
    send("reboot");
    std::this_thread::sleep_for(150ms);
    refreshing = false;
    ASSERT_TRUE(waitFor([&] { return calledTools().find("reboot \n") != std::string::npos; }, 5000ms));
}

TEST_F(RemoteCommandTest, BoardGateRejectionIsReported)
{
    shims.add("shareframe-remote-gate", "[ \"$1\" = check ] && { echo 'booted slot B is not confirmed yet'; exit 1; }");
    send("factory_reset");
    ASSERT_TRUE(waitStatuses({"rejected:booted slot B is not confirmed yet"}));
    EXPECT_EQ(calledTools().find("shareframe-factory-reset"), std::string::npos);
}

TEST_F(RemoteCommandTest, FactoryResetNeedsTheEvidenceUploaded)
{
    backend.on(std::string("POST ") + kDiagnostics, 503);
    send("factory_reset");
    ASSERT_TRUE(waitFor([&] { return !statuses().empty(); }, 5000ms));
    EXPECT_EQ(statuses()[0].rfind("rejected:diagnostics upload failed", 0), 0u) << statuses()[0];
    EXPECT_EQ(calledTools().find("shareframe-factory-reset"), std::string::npos);
}

TEST_F(RemoteCommandTest, UpdateInstallNeedsANewerRelease)
{
    newerRelease = false;
    send("update_install");
    ASSERT_TRUE(waitStatuses({"rejected:No newer version available"}));
    EXPECT_TRUE(backend.requests(kDiagnostics).empty());
}

TEST_F(RemoteCommandTest, UpdateInstallStartsTheUpdateAfterEvidence)
{
    send("update_install");
    ASSERT_TRUE(waitStatuses({"accepted"}));
    EXPECT_EQ(backend.requests(kDiagnostics).size(), 1u);
    EXPECT_TRUE(waitFor([&] { return calledTools().find("gate mark") != std::string::npos; }));
    EXPECT_NE(calledTools().find("gate check --no-rate-limit"), std::string::npos);
}

TEST_F(RemoteCommandTest, RequestDiagnosticsUploadsWithTheCommandId)
{
    send("request_diagnostics", "", 7);
    ASSERT_TRUE(waitStatuses({"accepted", "done"}));
    EXPECT_EQ(backend.requests(kDiagnostics).back().uri, std::string(kDiagnostics) + "?trigger=admin&command_id=7");
}

TEST_F(RemoteCommandTest, FailedFactoryResetHidesTheStatusScreenAgain)
{
    shims.add("shareframe-factory-reset", "exit 1");
    send("factory_reset");
    EXPECT_TRUE(waitFor([&] { return shownStatusScreens() == std::vector<std::string>{"factory_reset", "hide"}; }, 5000ms));
}

TEST_F(RemoteCommandTest, EnterSafeModeRunsTheBoardScript)
{
    send("enter_safe_mode");
    ASSERT_TRUE(waitStatuses({"accepted", "done"}));
    const auto tools = calledTools();
    EXPECT_NE(tools.find("shareframe-safe-mode enter"), std::string::npos);
    EXPECT_EQ(tools.find("gate "), std::string::npos);
    EXPECT_TRUE(backend.requests(kDiagnostics).empty());
}

TEST_F(RemoteCommandTest, ExitSafeModeRunsTheBoardScript)
{
    send("exit_safe_mode");
    ASSERT_TRUE(waitStatuses({"accepted", "done"}));
    EXPECT_NE(calledTools().find("shareframe-safe-mode exit"), std::string::npos);
}

TEST_F(RemoteCommandTest, SafeModeScriptRefusalIsReported)
{
    shims.add("shareframe-safe-mode", "echo 'booted slot B is not confirmed yet'; exit 1");
    send("enter_safe_mode");
    ASSERT_TRUE(waitStatuses({"accepted", "failed:booted slot B is not confirmed yet"}));
}

TEST_F(RemoteCommandTest, SafeModeTakesNoArgument)
{
    send("enter_safe_mode", "display");
    ASSERT_TRUE(waitStatuses({"rejected:invalid argument"}));
    EXPECT_EQ(calledTools().find("shareframe-safe-mode"), std::string::npos);
}

TEST_F(RemoteCommandTest, SafeModeNoOpIsReportedWithTheResult)
{
    shims.add("shareframe-safe-mode", "echo 'not in safe mode'");
    send("exit_safe_mode");
    ASSERT_TRUE(waitStatuses({"accepted", "done:not in safe mode"}));
}

TEST_F(RemoteCommandTest, NeverTogglesSafeModeWhileAnUpdateRuns)
{
    updatePhase = "installing";
    send("enter_safe_mode", "", 1);
    send("exit_safe_mode", "", 2);
    ASSERT_TRUE(waitStatuses({"rejected:update installing", "rejected:update installing"}));
    EXPECT_EQ(calledTools().find("shareframe-safe-mode"), std::string::npos);
}

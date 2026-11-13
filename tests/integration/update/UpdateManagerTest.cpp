#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "update/UpdateManager.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <mutex>

using namespace std::chrono_literals;

// The worker against a fake release server and fake board commands, up to the reboot.
namespace
{
constexpr auto kLatest = "/api/frame-updates/latest/";

class UpdateManagerTest : public ::testing::Test
{
protected:
    UpdateManagerTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.version = "1.2.0";
        storeValidToken(db.get());
        for (const auto* command : {"rauc", "shareframe-led", "tryboot-backend"})
            shims.add(command, "exit 0");
        shims.add("shareframe-update-history", "echo \"$*\" >> " + historyCalls.string());
        display.start();

        backend.on("GET /bundle.raucb", 200, "bundle");
        const nlohmann::json release = {
            {"version", "9.9.9"},
            {"criticality", "Low"},
            {"download_url", "http://" + backend.baseUrl() + "/bundle.raucb"},
        };
        backend.on(std::string("GET ") + kLatest, 200, release.dump());
    }

    ~UpdateManagerTest() override { display.stop(); }

    UpdateManager::Paths paths() const
    {
        const auto& dir = db.dir();
        return {(dir / "rauc/pending").string(), (dir / "rauc/pending-meta.json").string(),
                (dir / "update-history.jsonl").string(), (dir / "cache").string(),
                (dir / "tryboot").string(), "tryboot-backend"};
    }

    std::string phase() { return updates.status().value("phase", ""); }

    std::vector<std::string> statusScreens()
    {
        std::lock_guard lk(mtx);
        return screens;
    }

    std::string historyLog() const
    {
        std::ifstream f(historyCalls);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    FakeHttpServer backend;
    TestDb db;
    PathShim shims;
    std::filesystem::path historyCalls = db.dir() / "history-calls.log";
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    std::mutex mtx;
    std::vector<std::string> screens;
    std::string displayUrl = "ipc://" + (db.dir() / "display.sock").string();
    NngRepServer display{displayUrl, [this](const IpcMessage& msg) -> nlohmann::json {
        if (msg.type == IpcMessageType::ShowStatusScreen || msg.type == IpcMessageType::HideStatusScreen)
        {
            std::lock_guard lk(mtx);
            screens.push_back(msg.type == IpcMessageType::HideStatusScreen ? "hide" : msg.data.value("screen", ""));
        }
        return health::respond(msg);
    }};
    IpcClient displayIpc{displayUrl};
    UpdateManager updates{cfg, http, auth, displayIpc, paths()};
};
}

TEST_F(UpdateManagerTest, InstalledBundleWaitsForTheRebootIntoTheNewSlot)
{
    shims.add("s6-linux-init-shutdown", "exit 0");

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "awaiting-reboot"; }, 10000ms));

    EXPECT_TRUE(std::filesystem::exists(paths().trybootFlag));
    EXPECT_EQ(statusScreens(), std::vector<std::string>{"update"}) << "the screen has to stay up through the reboot";
    EXPECT_EQ(historyLog(), "");
}

// A phase that never ended made the frame refuse every remote reboot and update.
TEST_F(UpdateManagerTest, RebootThatFailsEndsTheUpdateInsteadOfWaitingForever)
{
    shims.add("s6-linux-init-shutdown", "echo 'shutdown refused' >&2; exit 1");

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "failed"; }, 10000ms)) << "stuck in " << phase();

    EXPECT_NE(updates.status().value("error", "").find("shutdown refused"), std::string::npos);
    EXPECT_EQ(statusScreens(), (std::vector<std::string>{"update", "hide"}));
    EXPECT_NE(historyLog().find("install-failed"), std::string::npos);
}

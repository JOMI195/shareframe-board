#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/Golden.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "update/UpdateManager.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class UpdateContractTest : public ::testing::Test
{
protected:
    UpdateContractTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.version = "1.2.0";
        storeValidToken(db.get());
        shims.add("shareframe-led", "exit 0");
        shims.add("shareframe-update-history", "exit 0");
        shims.add("tryboot-backend", "cat '" + golden::hardware("tryboot-get-primary.txt").string() + "'");
        display.start();
    }

    ~UpdateContractTest() override { display.stop(); }

    UpdateManager::Paths paths(const std::string& history) const
    {
        const auto& dir = db.dir();
        return {(dir / "rauc/pending").string(), (dir / "rauc/pending-meta.json").string(), history,
                (dir / "cache").string(), (dir / "tryboot").string(), "tryboot-backend", (dir / "cmdline").string()};
    }

    FakeHttpServer backend;
    TestDb db;
    PathShim shims;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    std::string displayUrl = "ipc://" + (db.dir() / "display.sock").string();
    NngRepServer display{displayUrl, health::respond};
    IpcClient displayIpc{displayUrl};
};
}

// The dashboard's update history shows what shareframe-update-history wrote, newest first.
TEST_F(UpdateContractTest, HistoryAndLastResultComeFromTheHardwareHistoryFile)
{
    UpdateManager updates{cfg, http, auth, displayIpc, paths(golden::hardware("update-history.jsonl").string())};

    const auto history = updates.history();
    ASSERT_EQ(history.size(), 3u);
    EXPECT_EQ(history[0].at("result"), "install-failed");
    EXPECT_EQ(history[1].at("result"), "rolled-back");
    EXPECT_EQ(history[2].at("result"), "committed");
    EXPECT_EQ(history[2].at("from_version"), "1.0.0");
    EXPECT_EQ(history[2].at("to_version"), "1.1.0");
    EXPECT_EQ(history[2].at("timestamp"), "2026-01-02T03:04:05");

    const auto last = updates.status().at("last_result");
    EXPECT_EQ(last.at("result"), "install-failed");
    EXPECT_EQ(last.at("error"), "rauc: \"bad signature\"");
}

TEST_F(UpdateContractTest, CommittedSlotIsWhatTheTrybootBackendPrints)
{
    UpdateManager updates{cfg, http, auth, displayIpc, paths((db.dir() / "history.jsonl").string())};
    EXPECT_EQ(updates.status().at("committed_slot"), "B");
}

// Staged before the reboot; shareframe-update-history and rauc-reconcile read it after.
TEST_F(UpdateContractTest, StagedMetaIsWhatTheHardwareHistoryReads)
{
    const nlohmann::json release = {{"version", "1.3.0"}, {"criticality", "Low"},
                                    {"download_url", "http://" + backend.baseUrl() + "/missing.raucb"}};
    backend.on("GET /api/frame-updates/latest/", 200, release.dump());
    UpdateManager updates{cfg, http, auth, displayIpc, paths((db.dir() / "history.jsonl").string())};

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return updates.status().value("phase", "") == "failed"; }, 10000ms));
    golden::expectBoard("pending-meta.json", golden::read(paths("").pendingMeta));
}

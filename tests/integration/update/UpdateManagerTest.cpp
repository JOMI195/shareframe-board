#include "auth/AuthTokenManager.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "update/UpdateManager.hpp"
#include "util/FileHash.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

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
        for (const auto* command : {"rauc", "tryboot-backend"})
            shims.add(command, "exit 0");
        shims.add("shareframe-led", "echo \"$*\" >> " + ledCalls.string());
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
                (dir / "tryboot").string(), "tryboot-backend", (dir / "cmdline").string()};
    }

    std::string phase() { return updates.status().value("phase", ""); }

    std::vector<std::string> statusScreens()
    {
        std::lock_guard lk(mtx);
        return screens;
    }

    static std::string read(const std::filesystem::path& path)
    {
        std::ifstream f(path);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    std::string historyLog() const { return read(historyCalls); }
    std::string ledLog() const { return read(ledCalls); }

    FakeHttpServer backend;
    TestDb db;
    PathShim shims;
    std::filesystem::path historyCalls = db.dir() / "history-calls.log";
    std::filesystem::path ledCalls = db.dir() / "led-calls.log";
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
    EXPECT_EQ(ledLog(), "updating\nupdate-done\n");
    EXPECT_EQ(historyLog(), "");
}

// An LED left on "updating" tells the owner not to unplug a frame that has long given up.
TEST_F(UpdateManagerTest, FailedInstallEndsTheUpdatingLed)
{
    shims.add("rauc", "echo 'bad signature' >&2; exit 1");

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "failed"; }, 10000ms)) << "stuck in " << phase();

    EXPECT_EQ(ledLog(), "updating\nupdate-failed\n");
    EXPECT_EQ(statusScreens(), (std::vector<std::string>{"update", "hide"}));
    EXPECT_FALSE(std::filesystem::exists(paths().trybootFlag));
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
    EXPECT_EQ(ledLog(), "updating\nupdate-done\nerror\n");
    EXPECT_NE(historyLog().find("install-failed"), std::string::npos);
}

// The dashboard and the heartbeat tell a trial boot by these two differing.
TEST_F(UpdateManagerTest, ReportsTheSlotItBootedAndTheOneTheFirmwareCommitted)
{
    std::ofstream(paths().cmdline) << "console=ttyS1,115200 rauc.slot=B panic=10\n";
    shims.add("tryboot-backend", "[ \"$1\" = get-primary ] && echo A");

    const auto status = updates.status();
    EXPECT_EQ(status.value("booted_slot", ""), "B");
    EXPECT_EQ(status.value("committed_slot", ""), "A");
}

namespace
{
void offer(FakeHttpServer& backend, nlohmann::json release)
{
    backend.on(std::string("GET ") + kLatest, 200, release.dump());
}
}

// A bundle that is not what the server announced never reaches rauc and leaves nothing on /data.
TEST_F(UpdateManagerTest, ABundleWithTheWrongChecksumIsDroppedAndRecorded)
{
    shims.add("rauc", "echo \"$*\" >> " + (db.dir() / "rauc-calls.log").string());
    offer(backend, {{"version", "9.9.9"}, {"criticality", "Low"}, {"checksum", std::string(64, '0')},
                    {"download_url", "http://" + backend.baseUrl() + "/bundle.raucb"}});

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "failed"; }, 10000ms));

    EXPECT_NE(updates.status().value("error", "").find("Checksum mismatch"), std::string::npos);
    EXPECT_NE(historyLog().find("install-failed Checksum mismatch"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(db.dir() / "rauc-calls.log"));
    EXPECT_TRUE(std::filesystem::is_empty(paths().cacheDir));
}

TEST_F(UpdateManagerTest, ABundleWithTheAnnouncedChecksumIsInstalled)
{
    shims.add("s6-linux-init-shutdown", "exit 0");
    const auto served = db.dir() / "served.raucb";
    std::ofstream(served) << "bundle";
    offer(backend, {{"version", "9.9.9"}, {"criticality", "Low"}, {"checksum", FileHash::sha256File(served.string())},
                    {"download_url", "http://" + backend.baseUrl() + "/bundle.raucb"}});

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    EXPECT_TRUE(waitFor([&] { return phase() == "awaiting-reboot"; }, 10000ms)) << "stuck in " << phase();
}

// The bundle is fetched with the frame's token, so it may only come from the frame's own server.
TEST_F(UpdateManagerTest, ABundleUrlOnAnotherHostIsFetchedFromTheFramesOwnServer)
{
    FakeHttpServer elsewhere;
    elsewhere.on("GET /bundle.raucb", 200, "bundle");
    offer(backend, {{"version", "9.9.9"}, {"criticality", "Low"},
                    {"download_url", "http://" + elsewhere.baseUrl() + "/bundle.raucb"}});
    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return backend.requests("/bundle.raucb").size() == 1; }, 10000ms));
    EXPECT_TRUE(elsewhere.requests().empty());
}

TEST_F(UpdateManagerTest, ARedirectAwayFromTheFramesServerIsNotFollowed)
{
    FakeHttpServer elsewhere;
    elsewhere.on("GET /bundle.raucb", 200, "bundle");
    backend.redirect("GET /bundle.raucb", "http://" + elsewhere.baseUrl() + "/bundle.raucb");
    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "failed"; }, 10000ms)) << phase();
    EXPECT_TRUE(elsewhere.requests().empty());
}

TEST_F(UpdateManagerTest, AReleaseWithoutDownloadUrlFailsWithoutDownloading)
{
    offer(backend, {{"version", "9.9.9"}, {"criticality", "Low"}});

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "failed"; }, 10000ms));
    EXPECT_EQ(updates.status().value("error", ""), "Release has no download_url");
}

// Another install over an unconfirmed slot would lose the way back.
TEST_F(UpdateManagerTest, AnUnconfirmedSlotBlocksTheNextUpdate)
{
    std::filesystem::create_directories(std::filesystem::path(paths().pendingFile).parent_path());
    std::ofstream(paths().pendingFile) << "B\n";

    std::string error;
    EXPECT_FALSE(updates.startUpdate(error));
    EXPECT_NE(error.find("awaits confirmation"), std::string::npos);
}

TEST_F(UpdateManagerTest, AnOlderOrEqualReleaseIsNotInstalled)
{
    offer(backend, {{"version", "1.2.0"}, {"criticality", "Low"},
                    {"download_url", "http://" + backend.baseUrl() + "/bundle.raucb"}});
    std::string error;
    EXPECT_FALSE(updates.startUpdate(error));
    EXPECT_EQ(error, "No newer version available");
}

// Bundles of skipped versions would fill /data over time.
TEST_F(UpdateManagerTest, StaleBundlesAreRemovedBeforeTheDownload)
{
    shims.add("s6-linux-init-shutdown", "exit 0");
    std::filesystem::create_directories(paths().cacheDir);
    std::ofstream(std::filesystem::path(paths().cacheDir) / "shareframe-9.0.0.raucb") << "old";
    std::ofstream(std::filesystem::path(paths().cacheDir) / "shareframe-9.1.0.raucb.part") << "old";

    std::string error;
    ASSERT_TRUE(updates.startUpdate(error)) << error;
    ASSERT_TRUE(waitFor([&] { return phase() == "awaiting-reboot"; }, 10000ms));
    EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(paths().cacheDir) / "shareframe-9.0.0.raucb"));
    EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(paths().cacheDir) / "shareframe-9.1.0.raucb.part"));
}

// Only the criticality the config names installs by itself; others wait for the owner.
TEST_F(UpdateManagerTest, ThePeriodicCheckInstallsOnlyTheConfiguredCriticality)
{
    shims.add("s6-linux-init-shutdown", "exit 0");
    cfg.update.checkIntervalSecs = 1;
    cfg.update.autoInstallCriticality = "Critical";
    updates.startPeriodicCheck();

    std::this_thread::sleep_for(2500ms);
    EXPECT_EQ(phase(), "idle") << "a Low release must wait for the owner";

    offer(backend, {{"version", "9.9.9"}, {"criticality", "Critical"},
                    {"download_url", "http://" + backend.baseUrl() + "/bundle.raucb"}});
    EXPECT_TRUE(waitFor([&] { return phase() == "awaiting-reboot"; }, 10000ms)) << "stuck in " << phase();
}

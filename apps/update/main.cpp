#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/HTTPClient.hpp"
#include "repository/TokenRepository.hpp"
#include "update/UpdateIpc.hpp"
#include "update/UpdateManager.hpp"
#include "Version.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.updateApplication.logFile);
    spdlog::info("shareframe-update v{} ({}) starting [profile: {}]", cfg.version, shareframe::kAppSha, profileName(profile));

    // Database (for AuthTokenManager)
    Database db;
    db.init(cfg.database, false);

    // HTTP client: long transfer timeout, release downloads are large
    HTTPClient http(60, 600);

    TokenRepository tokenRepo(db.get());
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    UpdateManager updates(cfg, http, authTokenManager);
    updates.startPeriodicCheck();

    // REP server: dashboard proxies its /api/system/updates/* routes here.
    UpdateIpc updateIpc(updates);
    NngRepServer repServer(cfg.ipc.updateRep, [&](const IpcMessage& msg) { return updateIpc.handle(msg); });
    repServer.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    repServer.stop();

    return 0;
}

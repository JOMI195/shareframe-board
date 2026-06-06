#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "events/EventBus.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include "repository/TokenRepository.hpp"
#include "task/Heartbeat.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.heartbeatApplication.logFile);
    spdlog::info("shareframe-heartbeat v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Database (for AuthTokenManager token cache)
    Database db;
    db.init(cfg.database);
    TokenRepository tokenRepo(db.get());

    // HTTP client: heartbeat POST + dashboard health probe
    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // IPC client: probes the application's health over its Unix socket
    IpcClient ipc(cfg.dashboardApplication.socketPath);

    // EventBus is required by the PeriodicTask base ctor; the heartbeat sends via
    // HTTP, not the bus, so it is intentionally left unstarted.
    EventBus eventBus;

    Heartbeat heartbeat(eventBus, cfg, http, authTokenManager, ipc);
    heartbeat.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    heartbeat.stop();
    ipc.disconnect();

    return 0;
}

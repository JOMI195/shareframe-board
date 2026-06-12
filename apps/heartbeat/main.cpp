#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "events/EventBus.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/NngRepServer.hpp"
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
    db.init(cfg.database, false);
    TokenRepository tokenRepo(db.get());

    // HTTP client: heartbeat POST + dashboard health probe
    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // IPC clients: probe the websocket + display + dashboard + update services over nng REP
    IpcClient wsIpc(cfg.ipc.wsRep);
    IpcClient displayIpc(cfg.ipc.displayRep);
    IpcClient dashboardIpc(cfg.ipc.dashboardRep);
    IpcClient updateIpc(cfg.ipc.updateRep);

    // EventBus is required by the PeriodicTask base ctor; the heartbeat sends via
    // HTTP, not the bus, so it is intentionally left unstarted.
    EventBus eventBus;

    Heartbeat heartbeat(eventBus, cfg, http, authTokenManager, wsIpc, displayIpc, dashboardIpc, updateIpc);
    heartbeat.start();

    // Health endpoint so other services can probe the heartbeat over nng.
    NngRepServer healthRep(cfg.ipc.heartbeatRep, health::respond);
    healthRep.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    healthRep.stop();
    heartbeat.stop();
    wsIpc.disconnect();
    displayIpc.disconnect();
    dashboardIpc.disconnect();
    updateIpc.disconnect();

    return 0;
}

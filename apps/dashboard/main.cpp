#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "dashboard/DashboardServer.hpp"
#include "dashboard/SessionManager.hpp"
#include "dashboard/WifiManager.hpp"
#include "db/Database.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/HTTPClient.hpp"
#include "repository/TokenRepository.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.dashboardApplication.logFile);
    spdlog::info("shareframe-dashboard v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Database (for AuthTokenManager)
    Database db;
    db.init(cfg.database, false);

    // Connect to the display service via IPC (will reconnect automatically)
    IpcClient ipc(cfg.ipc.displayRep);
    if (!ipc.connect())
        spdlog::warn("Display IPC endpoint not available yet, will retry on first request");

    // Setup HTTP client (for OTP verification and update check requests)
    HTTPClient http(60, 600);

    // Auth token manager (for update check endpoint)
    TokenRepository tokenRepo(db.get());
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // Managers
    WifiManager wifi;
    SessionManager sessions;

    // Update endpoints proxy to the shareframe-update service over nng
    IpcClient updateIpc(cfg.ipc.updateRep);
    if (!updateIpc.connect())
        spdlog::warn("Update service IPC endpoint not available yet, will retry on first request");

    DashboardServer server(cfg, ipc, http, sessions, authTokenManager, wifi, updateIpc);
    server.start();

    // Health endpoint so other services can probe the dashboard over nng.
    NngRepServer healthRep(cfg.ipc.dashboardRep, health::respond);
    healthRep.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    healthRep.stop();
    server.stop();
    updateIpc.disconnect();
    ipc.disconnect();

    return 0;
}

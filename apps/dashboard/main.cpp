#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "dashboard/DashboardServer.hpp"
#include "dashboard/SessionManager.hpp"
#include "dashboard/WifiManager.hpp"
#include "db/Database.hpp"
#include "ipc/IpcClient.hpp"
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

    // Connect to main service via IPC (will reconnect automatically on each request)
    IpcClient ipc(cfg.dashboardApplication.socketPath);
    if (!ipc.connect())
        spdlog::warn("Service IPC socket not available yet, will retry on first request");

    // Setup HTTP client (for OTP verification and update check requests)
    HTTPClient http(60, 600);

    // Auth token manager (for update check endpoint)
    TokenRepository tokenRepo(db.get());
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // Managers
    WifiManager wifi;
    SessionManager sessions;

    DashboardServer server(cfg, ipc, http, sessions, authTokenManager, wifi);
    server.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    server.stop();
    ipc.disconnect();

    return 0;
}

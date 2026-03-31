#include "app/Bootstrap.hpp"
#include "dashboard/DashboardServer.hpp"
#include "dashboard/SessionManager.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.dashboardApplication.logFile);
    spdlog::info("shareframe-dashboard v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Connect to main service via IPC
    IpcClient ipc(cfg.dashboardApplication.socketPath);
    if (!ipc.connect())
    {
        spdlog::error("Failed to connect to service IPC socket at {}", cfg.dashboardApplication.socketPath);
        return 1;
    }

    // Setup HTTP client (for OTP verification requests to ShareFrame server)
    HTTPClient http(60, 600);

    // Setup session manager and dashboard server
    SessionManager sessions;
    DashboardServer server(cfg, ipc, http, sessions);
    server.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    server.stop();
    ipc.disconnect();

    return 0;
}

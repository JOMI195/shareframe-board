#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/HTTPClient.hpp"
#include "repository/TokenRepository.hpp"
#include "update/UpdateManager.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.updateApplication.logFile);
    spdlog::info("shareframe-update v{} starting [profile: {}]", cfg.version, profileName(profile));

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
    NngRepServer repServer(cfg.ipc.updateRep, [&](const IpcMessage& msg) -> nlohmann::json
    {
        switch (msg.type)
        {
        case IpcMessageType::UpdateCheckLatest:
        {
            auto resp = updates.fetchLatestRelease();
            if (!resp.ok())
                return {{"ok", false}, {"error", resp.errorMsg}, {"status_code", resp.statusCode}};
            try
            {
                return {{"ok", true}, {"release", nlohmann::json::parse(resp.body)}};
            }
            catch (...)
            {
                return {{"ok", false}, {"error", "Invalid response from update server"}};
            }
        }

        case IpcMessageType::UpdatePerform:
        {
            std::string error;
            const bool started = updates.startUpdate(error);
            return {{"ok", started}, {"error", error}};
        }

        case IpcMessageType::UpdateStatus:
            return {{"ok", true}, {"status", updates.status()}};

        case IpcMessageType::UpdateHistory:
            return {{"ok", true}, {"history", updates.history()}};

        case IpcMessageType::GetHealth:
            return {{"running", true}};

        default:
            spdlog::warn("IPC: unhandled message type");
            return nlohmann::json::object();
        }
    });
    repServer.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    repServer.stop();

    return 0;
}

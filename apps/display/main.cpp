#include "app/Bootstrap.hpp"
#include "db/Database.hpp"
#include "display/DisplayIpc.hpp"
#include "display/DisplayManager.hpp"
#include "events/EventBus.hpp"
#include "ipc/EventSubscriber.hpp"
#include "ipc/NngRepServer.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/SettingsRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "task/DisplayClear.hpp"
#include "task/DisplayImageLoop.hpp"
#include <spdlog/spdlog.h>

// Owns all display behavior. Commands/queries arrive over nng REP, broadcast
// events over nng SUB; both feed the EventBus the display tasks consume.
int main(int argc, char* argv[])
{
    // Block SIGINT/SIGTERM before any thread is created so it reaches the
    // waitForSignal() thread instead of a worker (graceful shutdown).
    blockShutdownSignals();

    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.displayApplication.logFile);
    spdlog::info("shareframe-display v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Migrations run by the shareframe-migrate oneshot before any service; open
    // without migrating.
    Database database;
    database.init(cfg.database, false);
    ImageRepository imageRepo(database.get());
    SettingsRepository settingsRepo(database.get());
    DisplayMetricsRepository displayMetrics(database.get());
    RuntimeSettings runtimeSettings(settingsRepo, cfg);

    EventBus eventBus;

    DisplayManager displayManager(cfg, displayMetrics);
    if (!cfg.display.mockDisplay)
        displayMetrics.increment("app_boot_total");
    displayManager.init();

    // Display tasks subscribe to the bus — wire them before eventBus.start().
    DisplayImageLoop displayImageLoop(eventBus, cfg, imageRepo, displayManager, runtimeSettings);
    displayImageLoop.start();
    DisplayClear displayClear(eventBus, cfg, displayManager);
    displayClear.start();

    eventBus.start();

    DisplayIpc displayIpc(eventBus, runtimeSettings, displayImageLoop, imageRepo, displayManager);

    // REP: commands and queries from the dashboard.
    NngRepServer repServer(cfg.ipc.displayRep, [&](const IpcMessage& msg) { return displayIpc.handle(msg); });
    repServer.start();

    // SUB: broadcast events from the websocket service.
    EventSubscriber subscriber(cfg.ipc.wsPub, [&](const std::string& topic, const nlohmann::json& payload)
    {
        displayIpc.onEvent(topic, payload);
    });
    subscriber.start();

    const int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    // Wake the in-flight min-refresh wait first so the tasks below stop promptly.
    displayManager.requestShutdown();
    subscriber.stop();
    repServer.stop();
    eventBus.stop();
    displayClear.stop();
    displayImageLoop.stop();

    // Tasks stopped; leave a blank white screen (e-paper is bistable). No-op if
    // already cleared.
    displayManager.clearForShutdown();

    return 0;
}

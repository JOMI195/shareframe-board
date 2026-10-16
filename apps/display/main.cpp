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
#include "task/ApNetworkSetupScreen.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/DisplayTaskManager.hpp"
#include "task/NightBlankScreen.hpp"
#include "task/StatusScreen.hpp"
#include "task/WifiModeWatcher.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <string_view>

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
    // Safe mode (OS boot/crash loop guard): blank the panel once and leave it alone.
    if (std::find(argv + 1, argv + argc, std::string_view("--clear")) != argv + argc)
    {
        displayManager.init();
        spdlog::info("Panel cleared for safe mode");
        return 0;
    }
    if (!cfg.display.mockDisplay)
        displayMetrics.increment("app_boot_total");

    // Display tasks subscribe to the bus — wire them before eventBus.start(). The
    // manager boots the panel and starts/stops the content tasks itself.
    IpcClient wsIpc(cfg.ipc.wsRep);
    DisplayImageLoop displayImageLoop(eventBus, cfg, imageRepo, displayManager, runtimeSettings,
                                      [&] { return queryServerConnection(wsIpc); });
    ApNetworkSetupScreen apNetworkSetupScreen(eventBus, cfg, displayManager);
    NightBlankScreen nightBlankScreen(eventBus, displayManager);
    StatusScreen statusScreen(eventBus, cfg, displayManager);
    DisplayTaskManager displayTaskManager(eventBus, cfg, displayManager, displayImageLoop,
                                          apNetworkSetupScreen, nightBlankScreen, statusScreen, runtimeSettings);
    WifiModeWatcher wifiModeWatcher(eventBus, cfg);
    DisplayClear displayClear(eventBus, cfg, displayManager);
    displayImageLoop.subscribe();
    displayTaskManager.subscribe();
    displayClear.subscribe();
    displayClear.start();
    displayTaskManager.start();
    wifiModeWatcher.start();

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
    wifiModeWatcher.stop();
    eventBus.stop();
    displayTaskManager.stop();
    displayClear.stop();

    // Tasks stopped; e-paper is bistable, so leave the status screen of a reboot up,
    // otherwise a blank white screen (no-op if already cleared).
    if (displayTaskManager.state() == DisplayState::Status)
        statusScreen.finishForShutdown();
    else
        displayManager.clearForShutdown();

    return 0;
}

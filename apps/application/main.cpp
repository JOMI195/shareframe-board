#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "display/DisplayManager.hpp"
#include "events/EventBus.hpp"
#include "image/ImageManager.hpp"
#include "ipc/IpcServer.hpp"
#include "net/HTTPClient.hpp"
#include "net/WebsocketClient.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/SettingsRepository.hpp"
#include "repository/TokenRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "task/DisplayClear.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/ImageCheck.hpp"
#include "task/ImageUpdate.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.shareframeApplication.logFile);
    spdlog::info("shareframe-board v{} starting [profile: {}]", cfg.version, profileName(profile));

    // setup db
    Database database;
    database.init(cfg.database, true);
    TokenRepository tokenRepo(database.get());
    ImageRepository imageRepo(database.get());
    SettingsRepository settingsRepo(database.get());

    RuntimeSettings runtimeSettings(settingsRepo, cfg);
    ImageManager imageManager(cfg, imageRepo);
    DisplayManager displayManager(cfg);
    displayManager.init();

    // setup auth
    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // setup event bus
    EventBus eventBus;

    // setup IPC server for dashboard communication
    IpcServer ipcServer(eventBus, cfg, runtimeSettings);
    ipcServer.start();

    // setup tasks
    ImageUpdate imageUpdate(eventBus, imageManager, imageRepo);
    imageUpdate.start();
    DisplayImageLoop displayImageLoop(eventBus, cfg, imageRepo, displayManager, runtimeSettings);
    displayImageLoop.start();
    WebsocketClient wsClient(eventBus, cfg, authTokenManager);
    wsClient.start();

    // setup periodic tasks
    ImageCheck imageCheck(eventBus, cfg, imageRepo);
    imageCheck.start();
    DisplayClear displayClear(eventBus, cfg, displayManager);
    displayClear.start();

    // start eventbus
    eventBus.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    displayClear.stop();
    imageCheck.stop();
    ipcServer.stop();
    wsClient.stop();
    displayImageLoop.stop();
    imageUpdate.stop();
    eventBus.stop();
}

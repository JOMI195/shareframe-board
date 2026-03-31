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
#include "task/ConfigSender.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/Heartbeat.hpp"
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
    database.init(cfg.database);
    TokenRepository tokenRepo(database.get());
    ImageRepository imageRepo(database.get());
    SettingsRepository settingsRepo(database.get());
    RuntimeSettings runtimeSettings(settingsRepo, cfg);
    ImageManager imageManager(cfg, imageRepo);
    DisplayManager displayManager(cfg);
    displayManager.init();
    bool _ = displayManager.displayImage(cfg.display.loadingImagePath + "/logo-frame-loading-shareframe.jpg");

    // setup auth
    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // setup event bus, image update handler, and websocket
    EventBus bus;
    ImageUpdate imageUpdate(bus, imageManager, imageRepo);
    imageUpdate.start();
    DisplayImageLoop displayImageLoop(bus, cfg, imageRepo, displayManager, runtimeSettings);
    displayImageLoop.start();
    WebsocketClient wsClient(bus, cfg, authTokenManager);
    wsClient.start();

    // setup IPC server for dashboard communication
    IpcServer ipcServer(bus, cfg, runtimeSettings);
    ipcServer.start();

    // setup periodic tasks
    Heartbeat heartbeat(bus, cfg);
    ConfigSender configSender(bus, cfg);
    ImageCheck imageCheck(bus, cfg, imageRepo);
    heartbeat.start();
    configSender.start();
    imageCheck.start();

    int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    imageCheck.stop();
    configSender.stop();
    heartbeat.stop();
    ipcServer.stop();
    wsClient.stop();
    displayImageLoop.stop();
    imageUpdate.stop();
}

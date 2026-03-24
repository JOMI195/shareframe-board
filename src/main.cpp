#include "auth/AuthTokenManager.hpp"
#include "config/ConfigLoader.hpp"
#include "task/ConfigSender.hpp"
#include "db/Database.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/TokenRepository.hpp"
#include "image/ImageManager.hpp"
#include "events/EventBus.hpp"
#include "logging/ConsoleLogger.hpp"
#include "net/HTTPClient.hpp"
#include "logging/FileLogger.hpp"
#include "task/Heartbeat.hpp"
#include "task/ImageCheck.hpp"
#include "task/ImageUpdate.hpp"
#include "task/DisplayImageLoop.hpp"
#include "display/DisplayManager.hpp"
#include "net/WebsocketClient.hpp"
#include <signal.h>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>


int main(int argc, char* argv[])
{
    // set profile
    std::string profileStr = "dev";
    if (const char* e = std::getenv("SHAREFRAME_PROFILE"); e)
        profileStr = e;
    for (int i = 1; i < argc - 1; ++i)
    {
        if (std::string_view(argv[i]) == "--profile")
        {
            profileStr = argv[i + 1];
            break;
        }
    }

    // load config
    AppConfig cfg;
    Profile profile = Profile::Dev;
    try
    {
        profile = ConfigLoader::parseProfile(profileStr);
        cfg = ConfigLoader::load(profile);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[critical] Startup aborted: " << e.what() << '\n';
        return 1;
    }

    // setup logging
    std::unique_ptr<ILogger> logger;
    if (cfg.production)
        logger = std::make_unique<FileLogger>();
    else
        logger = std::make_unique<ConsoleLogger>();

    logger->init({
        .logDir = cfg.log.logPath,
        .logFullPath = cfg.log.logPath + "/" + cfg.shareframeApplication.logFile,
        .debug = cfg.debug,
    });

    spdlog::info("shareframe-board v{} starting [profile: {}]", cfg.version, profileName(profile));

    // setup db
    Database database;
    database.init(cfg.database);
    TokenRepository tokenRepo(database.get());
    ImageRepository imageRepo(database.get());
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
    DisplayImageLoop displayImageLoop(bus, cfg, imageRepo, displayManager);
    displayImageLoop.start();
    WebsocketClient wsClient(bus, cfg, authTokenManager);
    wsClient.start();

    // setup periodic tasks
    Heartbeat heartbeat(bus, cfg);
    ConfigSender configSender(bus, cfg);
    ImageCheck imageCheck(bus, cfg, imageRepo);
    heartbeat.start();
    configSender.start();
    imageCheck.start();

    // block until signal
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
    int sig;
    sigwait(&sigset, &sig);

    spdlog::info("Received signal {}, shutting down", sig);
    imageCheck.stop();
    configSender.stop();
    heartbeat.stop();
    wsClient.stop();
    displayImageLoop.stop();
    imageUpdate.stop();
}

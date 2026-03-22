#include "auth/AuthTokenManager.hpp"
#include "config/ConfigLoader.hpp"
#include "net/ConfigSender.hpp"
#include "db/Database.hpp"
#include "db/repository/TokenRepository.hpp"
#include "events/EventBus.hpp"
#include "logging/ConsoleLogger.hpp"
#include "net/HTTPClient.hpp"
#include "logging/FileLogger.hpp"
#include "net/Heartbeat.hpp"
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

    // setup auth
    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    // setup event bus and websocket
    EventBus bus;
    WebsocketClient wsClient(bus, cfg, authTokenManager);
    wsClient.start();

    // setup heartbeat and config sender
    Heartbeat heartbeat(bus, cfg);
    ConfigSender configSender(bus, cfg);
    heartbeat.start();
    configSender.start();

    // block until signal
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
    int sig;
    sigwait(&sigset, &sig);

    spdlog::info("Received signal {}, shutting down", sig);
    configSender.stop();
    heartbeat.stop();
    wsClient.stop();
}

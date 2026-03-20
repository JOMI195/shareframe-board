#include "logging/ConsoleLogger.hpp"
#include "logging/FileLogger.hpp"
#include "config/ConfigLoader.hpp"
#include "db/Database.hpp"
#include "db/repository/TokenRepository.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <ixwebsocket/IXWebSocket.h>

#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"

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
    {
        logger = std::make_unique<FileLogger>();
    }
    else
    {
        logger = std::make_unique<ConsoleLogger>();
    }

    logger->init({
        .logDir = cfg.log.logPath,
        .logFullPath = cfg.log.logPath + "/" + cfg.shareframeApplication.logFile,
        .debug = cfg.debug,
    });

    // setup db
    Database database;
    database.init(cfg.database);
    TokenRepository tokenRepo(database.get());

    ix::WebSocket ws;
    ws.setUrl(cfg.wsBaseUrl() + "/");

    ws.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Message)
            spdlog::info("Received: {}", msg->str);
    });

    ws.start();

    HTTPClient http = HTTPClient(60, 600);

    AuthTokenManager authTokenManager = AuthTokenManager(cfg, tokenRepo, http);
    bool succesfull = authTokenManager.init();
    std::map<std::string, std::string> headers = TokenAuth::buildTokenAuthHeaders(authTokenManager);
    for (const auto& [k, v] : headers)
        spdlog::debug("Header: {}: {}", k, v);

    spdlog::info("shareframe-board starting [profile: {}]", profileName(profile));
    return 0;
}

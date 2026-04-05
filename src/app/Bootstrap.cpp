#include "app/Bootstrap.hpp"
#include "config/ConfigLoader.hpp"
#include "logging/Logger.hpp"
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <string_view>

BootstrapResult bootstrap(const int argc, char* argv[])
{
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

    try
    {
        const auto profile = ConfigLoader::parseProfile(profileStr);
        auto cfg = ConfigLoader::load(profile);
        return {std::move(cfg), profile};
    }
    catch (const std::exception& e)
    {
        std::cerr << "[critical] Startup aborted: " << e.what() << '\n';
        std::exit(1);
    }
}

void initLogging(const AppConfig& cfg, const std::string& logFile)
{
    Logger::init({
        .logDir = cfg.log.logPath,
        .logFullPath = cfg.log.logPath + "/" + logFile,
        .debug = cfg.debug,
    });
}

int waitForSignal()
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
    int sig;
    sigwait(&sigset, &sig);
    return sig;
}

#include "logging/Logger.hpp"
#include <cstdio>
#include <filesystem>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void Logger::init(const LoggerParameters& params)
{
    try
    {
        std::filesystem::create_directories(params.logDir);
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "[warn] Failed to create log directory '%s': %s\n", params.logDir.c_str(), e.what());
    }

    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(2);
    bool hasFileSink = false;

    try
    {
        const auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            params.logFullPath, 5 * 1024 * 1024, 3);
        sinks.push_back(fileSink);
        hasFileSink = true;
    }
    catch (const std::exception& e)
    {
        // Log dir not writable; continue with console logging.
        std::fprintf(stderr, "[warn] Failed to create file logger sink '%s': %s\n", params.logFullPath.c_str(), e.what());
    }

    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    const auto logger = std::make_shared<spdlog::logger>("Main", sinks.begin(), sinks.end());
    logger->set_level(params.debug ? spdlog::level::debug : spdlog::level::info);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [" + params.processName + "] [%n] [%l] %v");
    // Flush info+ so logs are visible quickly in long-running services.
    logger->flush_on(spdlog::level::info);

    spdlog::set_default_logger(logger);
    spdlog::info(
        "Logger initialized - level={} ({}) path={}",
        params.debug ? "debug" : "info",
        hasFileSink ? "file+console" : "console-only",
        params.logFullPath);
}


#include "logging/FileLogger.hpp"
#include <filesystem>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void FileLogger::init(const LoggerParameters& params) {
    try {
        std::filesystem::create_directories(params.logDir);
    } catch (...) {}

    std::vector<spdlog::sink_ptr> sinks;

    try {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            params.logFullPath, 5 * 1024 * 1024, 3);
        sinks.push_back(fileSink);
    } catch (const std::exception&) {
        // Log dir not writable — fall through to console only
    }

    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    auto logger = std::make_shared<spdlog::logger>("shareframe", sinks.begin(), sinks.end());
    logger->set_level(params.debug ? spdlog::level::debug : spdlog::level::info);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::info("Logger initialized — level={} (file+console)", params.debug ? "debug" : "info");
}

#include "logging/ConsoleLogger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void ConsoleLogger::init(const LoggerParameters& params) {
    auto sink   = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("shareframe", sink);
    logger->set_level(params.debug ? spdlog::level::debug : spdlog::level::info);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::info("Logger initialized — level={} (console)", params.debug ? "debug" : "info");
}

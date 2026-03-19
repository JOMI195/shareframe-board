#pragma once
#include "logging/ILogger.hpp"

class ConsoleLogger : public ILogger {
public:
    void init(const LoggerParameters& params) override;
};

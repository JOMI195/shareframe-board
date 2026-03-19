#pragma once
#include "logging/ILogger.hpp"

class FileLogger : public ILogger {
public:
    void init(const LoggerParameters& params) override;
};

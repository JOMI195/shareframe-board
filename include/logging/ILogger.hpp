#pragma once
#include <string>

struct LoggerParameters {
    std::string logDir;
    std::string logFullPath;
    bool        debug = false;
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void init(const LoggerParameters& params) = 0;
};

#pragma once

#include <string>

struct LoggerParameters {
    std::string logDir;
    std::string logFullPath;
    bool debug = false;
};

class Logger {
public:
    static void init(const LoggerParameters& params);
};

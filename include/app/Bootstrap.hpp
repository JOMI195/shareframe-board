#pragma once
#include "config/AppConfig.hpp"
#include "config/Profile.hpp"
#include <string>

struct BootstrapResult
{
    AppConfig cfg;
    Profile profile;
};

/// Parses --profile from argv and SHAREFRAME_PROFILE env, loads config + secrets + version.
/// Prints to stderr and calls std::exit(1) on failure.
BootstrapResult bootstrap(int argc, char* argv[]);

/// Sets up spdlog (console or file logger depending on cfg.production).
void initLogging(const AppConfig& cfg, const std::string& logFile);

/// Blocks until SIGINT or SIGTERM is received. Returns the signal number.
int waitForSignal();

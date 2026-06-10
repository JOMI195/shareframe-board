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

/// Sets up spdlog with file logging (and console fallback if file sink fails).
void initLogging(const AppConfig& cfg, const std::string& logFile);

/// Blocks SIGINT + SIGTERM in the calling thread. MUST be called at the very start
/// of main(), before any worker thread is created, so every thread inherits the
/// blocked mask and the signal is delivered only to the thread in waitForSignal().
/// Without this, the kernel can deliver SIGTERM to a worker thread that does not
/// block it, killing the process before the graceful shutdown path runs.
void blockShutdownSignals();

/// Blocks until SIGINT or SIGTERM is received. Returns the signal number.
int waitForSignal();

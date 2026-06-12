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

/// Blocks SIGINT + SIGTERM in the calling thread. MUST be called at the start of
/// main() before any worker thread, so all threads inherit the blocked mask and
/// the signal reaches only waitForSignal() (not a worker that would kill the
/// process before graceful shutdown).
void blockShutdownSignals();

/// Blocks until SIGINT or SIGTERM is received. Returns the signal number.
int waitForSignal();

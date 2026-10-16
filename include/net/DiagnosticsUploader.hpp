#pragma once
#include <cstdint>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

struct AppConfig;
class AuthTokenManager;
class HTTPClient;

/// Collects the board diagnostics package (shareframe-diag, gzip) and posts it
/// to the backend. Shared by the dashboard button and remote commands.
class DiagnosticsUploader
{
public:
    struct Result
    {
        bool ok = false;
        std::string error;
    };

    DiagnosticsUploader(const AppConfig& cfg, const HTTPClient& http, AuthTokenManager& auth);

    /// Sends a leftover pre-reboot package first, then a fresh one.
    Result collectAndUpload(const std::string& trigger, std::optional<int64_t> commandId = std::nullopt) const;

    /// Persists a package to /data before a reboot-class action and uploads it;
    /// on upload failure the file stays for the next collectAndUpload.
    Result captureBeforeReboot(int64_t commandId) const;

private:
    Result _post(const std::string& package, const std::string& trigger, std::optional<int64_t> commandId) const;
    Result _uploadPending(std::optional<int64_t> commandId) const;

    const AppConfig& cfg_;
    const HTTPClient& http_;
    AuthTokenManager& auth_;
    std::shared_ptr<spdlog::logger> logger_;
};

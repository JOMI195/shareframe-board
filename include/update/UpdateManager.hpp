#pragma once
#include "config/AppConfig.hpp"
#include "net/HTTPClient.hpp"
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

class AuthTokenManager;

// Drives the A/B image update: check server, download bundle to /data/cache,
// rauc install, tryboot reboot. Slot state (pending/committed) is owned by the
// OS (tryboot-backend + rauc-reconcile/mark-good oneshots); this class only
// reads it for status reporting.
class UpdateManager
{
public:
    enum class Phase
    {
        Idle,
        Checking,
        Downloading,
        Installing,
        AwaitingReboot,
        Failed
    };

    UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager);
    ~UpdateManager();

    // Proxy of the server's latest-release metadata (TokenAuth).
    [[nodiscard]] HttpResponse fetchLatestRelease() const;

    // Start the async download+install worker. Fails when busy, when an
    // uncommitted update awaits confirmation, or when no newer version exists.
    bool startUpdate(std::string& error);

    // Periodic server check; auto-installs criticalities from cfg (Critical).
    void startPeriodicCheck();

    [[nodiscard]] nlohmann::json status() const;
    [[nodiscard]] nlohmann::json history() const;

    [[nodiscard]] static std::string currentVersion();
    [[nodiscard]] static bool isVersionNewer(const std::string& candidate, const std::string& current);

private:
    void _worker(nlohmann::json release);
    void _fail(const std::string& error, const std::string& toVersion);
    [[nodiscard]] std::optional<nlohmann::json> _latestReleaseJson() const;
    [[nodiscard]] static std::string _bootedSlot();
    [[nodiscard]] static std::string _pendingSlot();
    [[nodiscard]] static std::string _committedSlot();

    const AppConfig& cfg_;
    HTTPClient& http_;
    AuthTokenManager& authTokenManager_;
    std::shared_ptr<spdlog::logger> logger_;

    mutable std::mutex mutex_;
    Phase phase_ = Phase::Idle;
    std::string error_;
    std::string targetVersion_;
    long long expectedSize_ = 0;
    std::string downloadPath_;
    std::jthread worker_;
    std::jthread checker_;
};

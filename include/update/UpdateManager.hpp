#pragma once
#include "config/AppConfig.hpp"
#include "fsm/Machine.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include "update/UpdatePhase.hpp"
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

class AuthTokenManager;

// Drives the A/B update: check server, download bundle, rauc install, tryboot
// reboot. Slot state is owned by the OS; this class only reads it for status.
class UpdateManager
{
public:
    /// Where the update keeps its files on the board.
    struct Paths
    {
        std::string pendingFile = "/data/rauc/pending";
        std::string pendingMeta = "/data/rauc/pending-meta.json";
        std::string historyFile = "/data/shareframe/update-history.jsonl";
        std::string cacheDir = "/data/cache";
        std::string trybootFlag = "/run/tryboot";
        std::string trybootBackend = "/usr/lib/rauc/tryboot-backend";
    };

    UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager, IpcClient& displayIpc,
                  Paths paths);
    UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager, IpcClient& displayIpc);
    ~UpdateManager();

    // Proxy of the server's latest-release metadata (TokenAuth).
    [[nodiscard]] HttpResponse fetchLatestRelease() const;

    // Start the async download+install worker. Fails when busy, awaiting
    // confirmation, or no newer version exists.
    bool startUpdate(std::string& error);

    // Periodic server check; auto-installs criticalities from cfg (Critical).
    void startPeriodicCheck();

    [[nodiscard]] UpdatePhase phase() const { return phase_.state(); }
    [[nodiscard]] nlohmann::json status() const;
    [[nodiscard]] nlohmann::json history() const;

    [[nodiscard]] bool isNewerThanInstalled(const std::string& candidate) const;
    [[nodiscard]] static bool isVersionNewer(const std::string& candidate, const std::string& current);

private:
    void _worker(nlohmann::json release);
    void _fail(const std::string& error, const std::string& toVersion);
    [[nodiscard]] std::optional<nlohmann::json> _latestReleaseJson() const;
    [[nodiscard]] static std::string _bootedSlot();
    [[nodiscard]] std::string _pendingSlot() const;
    [[nodiscard]] std::string _committedSlot() const;

    const AppConfig& cfg_;
    HTTPClient& http_;
    AuthTokenManager& authTokenManager_;
    IpcClient& displayIpc_;
    Paths paths_;
    std::shared_ptr<spdlog::logger> logger_;

    mutable std::mutex mutex_;
    /// Moved under the lock only.
    fsm::Machine<UpdatePhase> phase_;
    std::string error_;
    std::string targetVersion_;
    std::atomic<size_t> downloadedBytes_{0};
    std::atomic<size_t> totalBytes_{0};
    std::jthread worker_;
    std::jthread checker_;
};

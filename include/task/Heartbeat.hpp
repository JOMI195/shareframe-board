#pragma once
#include "task/PeriodicTask.hpp"
#include <nlohmann/json.hpp>
#include <string>

class HTTPClient;
class AuthTokenManager;
class IpcClient;
class IImageRepository;

/// Periodically POSTs a heartbeat to the server with the frame's local IP,
/// version, process health, stored image count, and system metrics (from shareframe-sysinfo).
/// Runs as its own process (shareframe-heartbeat).
class Heartbeat : public PeriodicTask
{
public:
    Heartbeat(EventBus& bus, const AppConfig& cfg, const HTTPClient& http,
              AuthTokenManager& auth, IpcClient& wsIpc, IpcClient& displayIpc,
              IpcClient& dashboardIpc, IpcClient& updateIpc, IImageRepository& images);

protected:
    [[nodiscard]] int intervalSecs() const override;
    void execute() override;

private:
    static std::string _getLocalIp();
    nlohmann::json _getSysInfo() const;
    nlohmann::json _getNewEvents() const;
    void _saveEventCursor(int64_t seq) const;
    void _markBackendReached();

    const HTTPClient& http_;
    AuthTokenManager& auth_;
    IpcClient& wsIpc_;
    IpcClient& displayIpc_;
    IpcClient& dashboardIpc_;
    IpcClient& updateIpc_;
    IImageRepository& images_;
    bool backendReached_ = false;
};

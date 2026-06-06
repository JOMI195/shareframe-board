#pragma once
#include "task/PeriodicTask.hpp"
#include <string>

class HTTPClient;
class AuthTokenManager;
class IpcClient;

/// Periodically POSTs a heartbeat to the server (frame-hearbeat endpoint) with the
/// frame's local IP, version, and the running state of the application + dashboard
/// processes. Runs as its own process (shareframe-heartbeat).
class Heartbeat : public PeriodicTask
{
public:
    Heartbeat(EventBus& bus, const AppConfig& cfg, const HTTPClient& http,
              AuthTokenManager& auth, IpcClient& ipc);

protected:
    [[nodiscard]] int intervalSecs() const override;
    void execute() override;

private:
    static std::string _getLocalIp();
    bool _checkApplication() const;
    bool _checkDashboard() const;

    const HTTPClient& http_;
    AuthTokenManager& auth_;
    IpcClient& ipc_;
};

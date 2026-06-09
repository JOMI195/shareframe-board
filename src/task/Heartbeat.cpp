#include "task/Heartbeat.hpp"
#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <unistd.h>

Heartbeat::Heartbeat(EventBus& bus, const AppConfig& cfg, const HTTPClient& http,
                     AuthTokenManager& auth, IpcClient& wsIpc, IpcClient& displayIpc,
                     IpcClient& dashboardIpc)
    : PeriodicTask(bus, cfg, "Heartbeat"), http_(http), auth_(auth),
      wsIpc_(wsIpc), displayIpc_(displayIpc), dashboardIpc_(dashboardIpc)
{
}

int Heartbeat::intervalSecs() const
{
    return cfg_.heartbeat.intervalSecs;
}

void Heartbeat::execute()
{
    const auto localIp = _getLocalIp();
    if (localIp.empty())
    {
        logger_->warn("Could not determine local IP address; skipping heartbeat");
        return;
    }

    const bool wsRunning = health::isRunning(wsIpc_);
    const bool displayRunning = health::isRunning(displayIpc_);
    const bool dashboardRunning = health::isRunning(dashboardIpc_);
    const nlohmann::json payload = {
        {"local_ip_address", localIp},
        {"version", cfg_.version},
        // application_running kept for backend compatibility: the former monolith
        // is healthy iff both successor services answer.
        {"application_running", wsRunning && displayRunning},
        {"websocket_running", wsRunning},
        {"display_running", displayRunning},
        {"dashboard_running", dashboardRunning}
    };

    auto headers = TokenAuth::buildTokenAuthHeaders(auth_);
    if (headers.empty())
    {
        logger_->warn("No auth token available; skipping heartbeat");
        return;
    }
    headers["Content-Type"] = "application/json";

    const std::string url = cfg_.httpBaseUrl() + cfg_.heartbeat.httpUrl;
    const auto payloadDump = payload.dump();
    logger_->debug("Sending heartbeat to {}: {}", url, logging::summarizePayloadForLog(payloadDump, "heartbeat"));
    const auto res = http_.post(url, payloadDump, headers);

    if (res.statusCode == 401)
    {
        logger_->warn("Heartbeat unauthorized (401); invalidating token for refetch");
        auth_.invalidate();
        return;
    }
    if (!res.ok())
    {
        logger_->warn("Heartbeat failed: {} {}", res.statusCode, res.errorMsg);
        return;
    }
    logger_->debug("Heartbeat acknowledged ({})", res.statusCode);
}

std::string Heartbeat::_getLocalIp()
{
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return {};

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        close(fd);
        return {};
    }

    sockaddr_in localAddr{};
    socklen_t len = sizeof(localAddr);
    getsockname(fd, reinterpret_cast<sockaddr*>(&localAddr), &len);
    close(fd);

    char buf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &localAddr.sin_addr, buf, sizeof(buf));
    return buf;
}

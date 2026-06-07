#include "task/Heartbeat.hpp"
#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <unistd.h>

Heartbeat::Heartbeat(EventBus& bus, const AppConfig& cfg, const HTTPClient& http,
                     AuthTokenManager& auth, IpcClient& ipc)
    : PeriodicTask(bus, cfg, "Heartbeat"), http_(http), auth_(auth), ipc_(ipc)
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

    const nlohmann::json payload = {
        {"local_ip_address", localIp},
        {"version", cfg_.version},
        {"application_running", _checkApplication()},
        {"dashboard_running", _checkDashboard()}
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

bool Heartbeat::_checkApplication() const
{
    const auto resp = ipc_.sendAndReceive(
        {IpcMessageType::GetHealth, {}}, std::chrono::milliseconds{500});
    return resp.has_value() && resp->value("running", false);
}

bool Heartbeat::_checkDashboard() const
{
    const std::string url = "http://127.0.0.1:"
        + std::to_string(cfg_.dashboardApplication.port) + "/api/system/health";
    const auto res = http_.get(url);
    if (!res.ok())
        return false;
    try
    {
        return nlohmann::json::parse(res.body).value("running", false);
    }
    catch (...)
    {
        return false;
    }
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

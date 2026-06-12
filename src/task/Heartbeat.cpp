#include "task/Heartbeat.hpp"
#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include "util/Subprocess.hpp"
#include <arpa/inet.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

Heartbeat::Heartbeat(EventBus& bus, const AppConfig& cfg, const HTTPClient& http,
                     AuthTokenManager& auth, IpcClient& wsIpc, IpcClient& displayIpc,
                     IpcClient& dashboardIpc, IpcClient& updateIpc)
    : PeriodicTask(bus, cfg, "Heartbeat"), http_(http), auth_(auth),
      wsIpc_(wsIpc), displayIpc_(displayIpc), dashboardIpc_(dashboardIpc), updateIpc_(updateIpc)
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

    const bool wsRunning        = health::isRunning(wsIpc_);
    const bool displayRunning   = health::isRunning(displayIpc_);
    const bool dashboardRunning = health::isRunning(dashboardIpc_);
    const bool updateRunning    = health::isRunning(updateIpc_);
    nlohmann::json payload = {
        // Identity
        {"serial_number", cfg_.frameId},
        {"local_ip_address", localIp},
        {"version", cfg_.version},
        // Service health
        {"websocket_running", wsRunning},
        {"display_running", displayRunning},
        {"dashboard_running", dashboardRunning},
        {"update_running", updateRunning},
    };

    const auto sysInfo = _getSysInfo();
    static const std::vector<std::string> sysInfoKeys = {
        // System state
        "health_state", "uptime_seconds", "boot_count", "boot_slot",
        "time_iso", "kernel", "fw_version",
        // CPU
        "cpu_temp_celsius", "cpu_usage_percent", "cpu_freq_mhz",
        "load_1", "load_5", "load_15",
        // RAM
        "ram_total_bytes", "ram_available_bytes",
        // Storage
        "storage_data_total_bytes", "storage_data_free_bytes",
    };
    for (const auto& key : sysInfoKeys)
    {
        if (sysInfo.contains(key))
            payload[key] = sysInfo[key];
    }

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

nlohmann::json Heartbeat::_getSysInfo() const
{
    nlohmann::json result = nlohmann::json::object();
    auto info = Subprocess::run({"shareframe-sysinfo"}, 10);
    if (info.exitCode != 0)
    {
        logger_->warn("shareframe-sysinfo failed: {}", info.stdErr);
        return result;
    }
    std::istringstream stream(info.stdOut);
    std::string line;
    while (std::getline(stream, line))
    {
        auto eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        const auto key = line.substr(0, eq);
        const auto val = line.substr(eq + 1);
        // Coerce numeric/bool strings to typed JSON scalars.
        if (val == "true")  { result[key] = true;  continue; }
        if (val == "false") { result[key] = false; continue; }
        try { std::size_t p; long long i = std::stoll(val, &p); if (p == val.size()) { result[key] = i; continue; } } catch (...) {}
        try { std::size_t p; double d  = std::stod(val, &p);  if (p == val.size()) { result[key] = d; continue; } } catch (...) {}
        result[key] = val;
    }
    return result;
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

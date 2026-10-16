#include "task/Heartbeat.hpp"
#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include "util/Subprocess.hpp"
#include "Version.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <filesystem>
#include <fstream>
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
    if (backendReached_)
        return cfg_.heartbeat.intervalSecs;
    return std::min(cfg_.heartbeat.firstContactRetrySecs, cfg_.heartbeat.intervalSecs);
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
        {"app_sha", shareframe::kAppSha},
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
        "time_iso", "kernel", "os_sha",
        // CPU
        "cpu_temp_celsius", "cpu_usage_percent", "cpu_freq_mhz",
        "load_1", "load_5", "load_15",
        // RAM
        "ram_total_bytes", "ram_available_bytes",
        // Storage
        "storage_data_total_bytes", "storage_data_free_bytes", "data_written_kb", "sd_card",
        // Faults & watermarks
        "health_oom", "health_temp", "health_disk",
        "crash_count", "unclean_resets", "oom_kills", "uv_events",
        "temp_peak_celsius", "ram_available_min_bytes", "slot_trial", "safe_mode",
        // WiFi (no SSID)
        "wifi_mode", "wlan_connected", "wlan_signal_dbm",
    };
    for (const auto& key : sysInfoKeys)
    {
        if (sysInfo.contains(key))
            payload[key] = sysInfo[key];
    }
    for (const auto& [key, value] : sysInfo.items())
    {
        if (key.starts_with("proc_"))
            payload[key] = value;
    }

    if (displayRunning)
    {
        if (const auto stats = displayIpc_.sendAndReceive({IpcMessageType::GetDisplayStats, {}}, std::chrono::milliseconds{1000});
            stats && stats->is_object())
        {
            for (const auto& [key, value] : stats->items())
                payload["display_" + key] = value;
        }
    }

    const auto events = _getNewEvents();
    if (!events.empty())
        payload["events"] = events;

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
    if (!backendReached_)
        _markBackendReached();
    if (!events.empty())
        _saveEventCursor(events.back()["seq"].get<int64_t>());
}

void Heartbeat::_markBackendReached()
{
    backendReached_ = true;
    if (!std::ofstream{cfg_.heartbeat.backendOkPath})
        logger_->warn("Could not write {}", cfg_.heartbeat.backendOkPath);
}

nlohmann::json Heartbeat::_getNewEvents() const
{
    constexpr std::size_t kMaxEventsPerBeat = 50;
    auto events = nlohmann::json::array();

    int64_t cursor = 0;
    std::ifstream(cfg_.heartbeat.eventCursorPath) >> cursor;
    const auto res = Subprocess::run({"shareframe-event", "--since", std::to_string(cursor)}, 10);
    if (res.exitCode != 0)
        return events;

    // Line format: "<seq> <time> [<source>] <message>"
    std::istringstream stream(res.stdOut);
    std::string line;
    while (events.size() < kMaxEventsPerBeat && std::getline(stream, line))
    {
        std::istringstream fields(line);
        int64_t seq = 0;
        std::string time, source;
        if (!(fields >> seq >> time >> source) || source.size() < 2)
            continue;
        std::string message;
        std::getline(fields >> std::ws, message);
        // The backend logs the body as a Python bytes repr that Grafana re-parses as JSON.
        for (auto& c : message)
            if (c < 0x20 || c > 0x7e || c == '"' || c == '\'' || c == '\\')
                c = ' ';
        events.push_back({{"seq", seq}, {"time", time},
                          {"source", source.substr(1, source.size() - 2)}, {"message", message}});
    }
    return events;
}

void Heartbeat::_saveEventCursor(int64_t seq) const
{
    const auto tmp = cfg_.heartbeat.eventCursorPath + ".tmp";
    if (std::ofstream(tmp) << seq << '\n')
    {
        std::error_code ec;
        std::filesystem::rename(tmp, cfg_.heartbeat.eventCursorPath, ec);
        if (ec)
            logger_->warn("Could not save event cursor: {}", ec.message());
    }
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

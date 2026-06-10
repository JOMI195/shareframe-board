#include "dashboard/SystemHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "util/Subprocess.hpp"

#include <sstream>

using namespace dashboard;

namespace {
// Turn a shareframe-sysinfo "value" token into a typed JSON scalar so the
// frontend receives numbers/bools it can render directly (not strings).
nlohmann::json coerceScalar(const std::string& v)
{
    if (v == "true")  return true;
    if (v == "false") return false;
    try { std::size_t p; long long i = std::stoll(v, &p); if (p == v.size()) return i; } catch (...) {}
    try { std::size_t p; double d = std::stod(v, &p);     if (p == v.size()) return d; } catch (...) {}
    return v;
}
} // namespace

SystemHandlers::SystemHandlers(const AppConfig& cfg, WifiManager& wifi)
    : cfg_(cfg), wifi_(wifi),
      logger_(spdlog::default_logger()->clone("SystemHandlers")),
      // Stable short ids form the public log contract (the frontend's
      // ServiceType enum); each maps to its spdlog file below in handleLogs.
      // Decoupled from internal *.service names / log filenames on purpose.
      allowedServiceNames_({"websocket", "display", "dashboard", "heartbeat", "update", "system"})
{
}

ix::HttpResponsePtr SystemHandlers::handleHealth(const ix::HttpRequestPtr& /*req*/) const
{
    // Health hook: serving this request means the dashboard is running. Always
    // true for now; can later reflect internal state.
    return jsonResponse(200, "OK", {{"running", true}});
}

ix::HttpResponsePtr SystemHandlers::handleInfo(const ix::HttpRequestPtr& /*req*/) const
{
    nlohmann::json data = {
        {"serial_number", cfg_.frameId},
        {"version", cfg_.version},
    };

    // Board-specific metrics come from the shareframe-sysinfo overlay script
    // (key=value lines). Absent metrics are simply missing keys -> treated as
    // null by the frontend; never fail the whole response over them.
    auto info = Subprocess::run({"shareframe-sysinfo"}, 10);
    if (info.exitCode == 0)
    {
        std::istringstream stream(info.stdOut);
        std::string line;
        while (std::getline(stream, line))
        {
            auto eq = line.find('=');
            if (eq == std::string::npos || eq == 0) continue;
            data[line.substr(0, eq)] = coerceScalar(line.substr(eq + 1));
        }
    }
    else
    {
        logger_->warn("shareframe-sysinfo failed: {}", info.stdErr);
    }

    // SSID can contain spaces, so source it from the wifi parser rather than
    // the script's key=value output.
    auto wifi = wifi_.getCurrentConnection();
    data["wlan_ssid"] = wifi.value("connection_name", "");

    return jsonResponse(200, "OK", data);
}

ix::HttpResponsePtr SystemHandlers::handleCheckInternet(const ix::HttpRequestPtr& /*req*/) const
{
    auto result = Subprocess::run({"ping", "-c", "2", "-w", "3", "1.1.1.1"}, 10);

    return jsonResponse(200, "OK", {
        {"connected", result.exitCode == 0},
        {"output", result.stdOut}
    });
}

ix::HttpResponsePtr SystemHandlers::handleRestart(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System reboot requested");
    // Clean s6 shutdown: signals shutdownd to run rc.shutdown (stop services,
    // unmount /data, mark RAUC slot) before rebooting. NOT busybox reboot,
    // which would bypass all of that.
    auto result = Subprocess::run({"s6-linux-init-shutdown", "-r", "now"}, 5);

    if (result.exitCode != 0)
    {
        logger_->error("Reboot failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Reboot failed");
    }

    return jsonResponse(200, "OK", nlohmann::json::object(), "Rebooting...");
}

ix::HttpResponsePtr SystemHandlers::handleShutdown(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System shutdown requested");
    auto result = Subprocess::run({"s6-linux-init-shutdown", "-p", "now"}, 5);

    if (result.exitCode != 0)
    {
        logger_->error("Shutdown failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Shutdown failed");
    }

    return jsonResponse(200, "OK", nlohmann::json::object(), "Shutting down...");
}

ix::HttpResponsePtr SystemHandlers::handleLogs(
    const ix::HttpRequestPtr& /*req*/,
    const std::unordered_map<std::string, std::string>& queryParams) const
{
    // Extract and validate service_name
    std::string serviceName;
    if (auto it = queryParams.find("service_name"); it != queryParams.end())
        serviceName = it->second;

    if (!Validation::isAllowedServiceName(serviceName, allowedServiceNames_))
        return errorResponse(400, "Bad Request", "Invalid or unknown service_name");

    // Extract and clamp lines
    int lines = Validation::DEFAULT_LOG_LINES;
    if (auto it = queryParams.find("lines"); it != queryParams.end())
    {
        try { lines = std::stoi(it->second); }
        catch (...) {}
    }
    lines = Validation::clampLogLines(lines);

    // No journald on the board. App services are spdlog rotating files under
    // cfg.log.logPath; "system" is the busybox syslog (klogd + daemons) at its
    // own absolute path. Map the (already-validated) short id to a full path.
    std::string path;
    if (serviceName == "dashboard")
        path = cfg_.log.logPath + "/" + cfg_.dashboardApplication.logFile;
    else if (serviceName == "websocket")
        path = cfg_.log.logPath + "/" + cfg_.websocketApplication.logFile;
    else if (serviceName == "display")
        path = cfg_.log.logPath + "/" + cfg_.displayApplication.logFile;
    else if (serviceName == "heartbeat")
        path = cfg_.log.logPath + "/" + cfg_.heartbeatApplication.logFile;
    else if (serviceName == "update")
        path = cfg_.log.logPath + "/" + cfg_.updateApplication.logFile;
    else if (serviceName == "system")
        path = cfg_.log.systemLogPath;
    auto result = Subprocess::run({"tail", "-n", std::to_string(lines), path}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("tail {} failed: {}", path, result.stdErr);
        return errorResponse(500, "Internal Server Error", "Failed to retrieve logs");
    }

    return jsonResponse(200, "OK", {
        {"service_name", serviceName},
        {"lines", lines},
        {"logs", result.stdOut}
    });
}


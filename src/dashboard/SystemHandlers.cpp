#include "dashboard/SystemHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "util/Subprocess.hpp"
#include "util/SysInfo.hpp"
#include "Version.hpp"

using namespace dashboard;

SystemHandlers::SystemHandlers(const AppConfig& cfg, WifiManager& wifi, const DiagnosticsUploader& diagnostics,
                               IpcClient& displayIpc)
    : cfg_(cfg), wifi_(wifi), diagnostics_(diagnostics), displayIpc_(displayIpc),
      logger_(spdlog::default_logger()->clone("SystemHandlers")),
      // Stable short ids = the public log contract; mapped to files in handleLogs.
      allowedServiceNames_({"websocket", "display", "dashboard", "heartbeat", "update", "system", "events"})
{
}

ix::HttpResponsePtr SystemHandlers::handleHealth(const ix::HttpRequestPtr& /*req*/) const
{
    // Serving this means the dashboard is running; always true for now.
    return jsonResponse(200, "OK", {{"running", true}});
}

ix::HttpResponsePtr SystemHandlers::handleInfo(const ix::HttpRequestPtr& /*req*/) const
{
    nlohmann::json data = {
        {"serial_number", cfg_.frameId},
        {"version", cfg_.version},
    };

    // Board metrics from the shareframe-sysinfo overlay (key=value); absent keys
    // are just missing — never fail the whole response.
    {
        constexpr auto kFresh = std::chrono::seconds(5);
        std::lock_guard lk(sysinfoMtx_);
        const auto now = std::chrono::steady_clock::now();
        if (sysinfo_.is_null() || now - sysinfoAt_ >= kFresh)
        {
            if (auto info = Subprocess::run({"shareframe-sysinfo"}, 10); info.exitCode == 0)
            {
                sysinfo_ = SysInfo::parse(info.stdOut);
                sysinfoAt_ = now;
            }
            else
            {
                logger_->warn("shareframe-sysinfo failed: {}", info.stdErr);
                sysinfo_ = nullptr;
            }
        }
        if (!sysinfo_.is_null())
            data.update(sysinfo_);
    }

    // Compiled in, so it wins over the sysinfo key.
    data["app_sha"] = shareframe::kAppSha;

    // SSID can contain spaces, so take it from the wifi parser, not key=value.
    auto wifi = wifi_.getCurrentConnection();
    data["wlan_ssid"] = wifi.value("connection_name", "");

    return jsonResponse(200, "OK", data);
}

ix::HttpResponsePtr SystemHandlers::handleRestart(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System reboot requested");
    // /sbin/reboot is the s6 clean wrapper (shutdownd -> rc.shutdown), not busybox.
    if (!_runWithStatusScreen(StatusScreenKind::Reboot, {"reboot"}))
        return errorResponse(500, "Internal Server Error", "Reboot failed");

    return jsonResponse(200, "OK", nlohmann::json::object(), "Rebooting...");
}

ix::HttpResponsePtr SystemHandlers::handleShutdown(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System shutdown requested");
    auto result = Subprocess::run({"shutdown"}, 5);

    if (result.exitCode != 0)
    {
        logger_->error("Shutdown failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Shutdown failed");
    }

    return jsonResponse(200, "OK", nlohmann::json::object(), "Shutting down...");
}

ix::HttpResponsePtr SystemHandlers::handleExitSafeMode(const ix::HttpRequestPtr& /*req*/) const
{
    const auto res = Subprocess::run({"shareframe-safe-mode", "exit"}, 15);
    auto out = res.stdOut.empty() ? res.stdErr : res.stdOut;
    out.erase(out.find_last_not_of(" \n\r\t") + 1);
    if (res.exitCode != 0)
    {
        logger_->error("Leaving safe mode failed: {}", out);
        return jsonResponse(500, "Internal Server Error", {{"reason", out}}, "Leaving safe mode failed");
    }
    logger_->info("Safe mode lifted from the dashboard: {}", out);
    return jsonResponse(200, "OK", nlohmann::json::object(), out);
}

ix::HttpResponsePtr SystemHandlers::handleSendDiagnostics(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("Diagnostics upload requested by owner");
    const auto result = diagnostics_.collectAndUpload("owner");
    if (!result.ok)
    {
        logger_->error("Diagnostics upload failed: {}", result.error);
        return errorResponse(502, "Bad Gateway", result.error);
    }
    return jsonResponse(200, "OK", nlohmann::json::object(), "Diagnostics sent");
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

    // No journald: app services are spdlog files; "system" is the busybox syslog.
    // Map the validated short id to a path.
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
    else if (serviceName == "events")
        path = cfg_.log.eventLogPath;
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

bool SystemHandlers::_runWithStatusScreen(const StatusScreenKind kind, const std::vector<std::string>& command) const
{
    displayIpc_.send({IpcMessageType::ShowStatusScreen, {{"screen", toString(kind)}}});
    const auto result = Subprocess::run(command, 5);
    if (result.exitCode == 0)
        return true;
    logger_->error("{} failed: {}", command.front(), result.stdErr);
    displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
    return false;
}

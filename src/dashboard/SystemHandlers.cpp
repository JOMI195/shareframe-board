#include "dashboard/SystemHandlers.hpp"
#include "auth/TokenAuth.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "util/Subprocess.hpp"

using namespace dashboard;

SystemHandlers::SystemHandlers(const AppConfig& cfg, HTTPClient& http,
                               AuthTokenManager& authTokenManager)
    : cfg_(cfg), http_(http), authTokenManager_(authTokenManager),
      logger_(spdlog::default_logger()->clone("SystemHandlers")),
      allowedServiceNames_({
          cfg.shareframeApplication.serviceName,
          cfg.dashboardApplication.serviceName
      })
{
}

ix::HttpResponsePtr SystemHandlers::handleInfo(const ix::HttpRequestPtr& /*req*/) const
{
    return jsonResponse(200, "OK", {
        {"serial_number", cfg_.secrets.publicSerialNumber},
        {"version", cfg_.version}
    });
}

ix::HttpResponsePtr SystemHandlers::handleCheckInternet(const ix::HttpRequestPtr& /*req*/) const
{
    auto result = Subprocess::run({"ping", "-c", "4", "1.1.1.1"}, 10);

    return jsonResponse(200, "OK", {
        {"connected", result.exitCode == 0},
        {"output", result.stdOut}
    });
}

ix::HttpResponsePtr SystemHandlers::handleRestart(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System reboot requested");
    auto result = Subprocess::run({"sudo", "systemctl", "reboot"}, 5);

    if (result.exitCode != 0)
    {
        logger_->error("Reboot failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Reboot failed");
    }

    return jsonResponse(200, "OK", {{"message", "Rebooting..."}});
}

ix::HttpResponsePtr SystemHandlers::handleShutdown(const ix::HttpRequestPtr& /*req*/) const
{
    logger_->info("System shutdown requested");
    auto result = Subprocess::run({"sudo", "shutdown", "now"}, 5);

    if (result.exitCode != 0)
    {
        logger_->error("Shutdown failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Shutdown failed");
    }

    return jsonResponse(200, "OK", {{"message", "Shutting down..."}});
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

    // Build journalctl command
    std::vector<std::string> args = {
        "sudo", "journalctl",
        "-u", serviceName,
        "-n", std::to_string(lines),
        "--output=short", "--no-pager"
    };

    // Optional since filter
    if (auto it = queryParams.find("since"); it != queryParams.end() && !it->second.empty())
    {
        args.push_back("--since");
        args.push_back(it->second);
    }

    auto result = Subprocess::run(args, 15);

    if (result.exitCode != 0)
    {
        logger_->error("journalctl failed: {}", result.stdErr);
        return errorResponse(500, "Internal Server Error", "Failed to retrieve logs");
    }

    return jsonResponse(200, "OK", {
        {"service_name", serviceName},
        {"lines", lines},
        {"logs", result.stdOut}
    });
}

ix::HttpResponsePtr SystemHandlers::handleLatestUpdate(const ix::HttpRequestPtr& /*req*/) const
{
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authTokenManager_);
    if (authHeaders.empty())
    {
        logger_->error("Failed to build token auth headers");
        return errorResponse(500, "Internal Server Error", "Auth token unavailable");
    }

    HTTPClient::Headers headers;
    for (auto& [k, v] : authHeaders)
        headers[k] = v;

    std::string url = cfg_.httpBaseUrl() + cfg_.update.httpLatestUrl;
    auto resp = http_.get(url, headers);

    if (!resp.ok())
    {
        logger_->error("Update check failed: {} {}", resp.statusCode, resp.errorMsg);
        return errorResponse(502, "Bad Gateway", "Failed to check for updates");
    }

    try
    {
        auto data = nlohmann::json::parse(resp.body);
        return jsonResponse(200, "OK", data);
    }
    catch (...)
    {
        return errorResponse(502, "Bad Gateway", "Invalid response from update server");
    }
}

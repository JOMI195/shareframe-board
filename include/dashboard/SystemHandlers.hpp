#pragma once
#include "config/AppConfig.hpp"
#include "dashboard/WifiManager.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "net/DiagnosticsUploader.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

class SystemHandlers
{
public:
    SystemHandlers(const AppConfig& cfg, WifiManager& wifi, const DiagnosticsUploader& diagnostics,
                   IpcClient& displayIpc);

    ix::HttpResponsePtr handleHealth(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleInfo(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleCheckInternet(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleRestart(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleShutdown(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleFactoryReset(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleSendDiagnostics(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleLogs(const ix::HttpRequestPtr& req,
                                   const std::unordered_map<std::string, std::string>& queryParams) const;

private:
    /// Shows the status screen, runs the command and hides the screen again if it failed.
    [[nodiscard]] bool _runWithStatusScreen(StatusScreenKind kind, const std::vector<std::string>& command) const;

    const AppConfig& cfg_;
    WifiManager& wifi_;
    const DiagnosticsUploader& diagnostics_;
    IpcClient& displayIpc_;
    std::shared_ptr<spdlog::logger> logger_;
    std::vector<std::string> allowedServiceNames_;
};

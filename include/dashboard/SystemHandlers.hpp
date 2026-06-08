#pragma once
#include "config/AppConfig.hpp"
#include "dashboard/WifiManager.hpp"
#include "net/HTTPClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

class AuthTokenManager;

class SystemHandlers
{
public:
    SystemHandlers(const AppConfig& cfg, HTTPClient& http,
                   AuthTokenManager& authTokenManager, WifiManager& wifi);

    ix::HttpResponsePtr handleHealth(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleInfo(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleCheckInternet(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleRestart(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleShutdown(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleLogs(const ix::HttpRequestPtr& req,
                                   const std::unordered_map<std::string, std::string>& queryParams) const;
    ix::HttpResponsePtr handleLatestUpdate(const ix::HttpRequestPtr& req) const;

private:
    const AppConfig& cfg_;
    HTTPClient& http_;
    AuthTokenManager& authTokenManager_;
    WifiManager& wifi_;
    std::shared_ptr<spdlog::logger> logger_;
    std::vector<std::string> allowedServiceNames_;
};

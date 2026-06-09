#pragma once
#include "dashboard/WifiManager.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

class WifiHandlers
{
public:
    explicit WifiHandlers(WifiManager& wifi);

    ix::HttpResponsePtr handleStatus(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleMode(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleSavedNetworks(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleConnect(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleForget(const ix::HttpRequestPtr& req) const;

private:
    WifiManager& wifi_;
    std::shared_ptr<spdlog::logger> logger_;
};

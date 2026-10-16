#pragma once
#include "dashboard/WifiManager.hpp"
#include "ipc/IpcClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

class WifiHandlers
{
public:
    WifiHandlers(WifiManager& wifi, IpcClient& wsIpc);

    ix::HttpResponsePtr handleStatus(const ix::HttpRequestPtr& req) const;
    /// The wifi mode plus the server link; server_connected is null when the websocket service is silent.
    nlohmann::json connectionMode() const;
    ix::HttpResponsePtr handleSavedNetworks(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleConnect(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleForget(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleSetApPassword(const ix::HttpRequestPtr& req) const;

private:
    WifiManager& wifi_;
    IpcClient& wsIpc_;
    std::shared_ptr<spdlog::logger> logger_;
};

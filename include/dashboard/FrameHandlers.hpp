#pragma once
#include "config/AppConfig.hpp"
#include "ipc/IpcClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

class FrameHandlers
{
public:
    FrameHandlers(IpcClient& ipc, AppConfig& cfg);

    ix::HttpResponsePtr handleStatus(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleControl(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleUpdateInterval(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleSkip(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleClear(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleDisplayStats(const ix::HttpRequestPtr& req) const;

private:
    IpcClient& ipc_;
    AppConfig& cfg_;
    std::shared_ptr<spdlog::logger> logger_;
};

#pragma once
#include "ipc/IpcClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

// Proxies the /api/system/updates/* routes to the shareframe-update service
// over nng (the installer must outlive dashboard restarts).
class UpdateHandlers
{
public:
    explicit UpdateHandlers(IpcClient& updateIpc);

    ix::HttpResponsePtr handleLatest(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handlePerformUpdate(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleStatus(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleHistory(const ix::HttpRequestPtr& req) const;

private:
    IpcClient& updateIpc_;
    std::shared_ptr<spdlog::logger> logger_;
};

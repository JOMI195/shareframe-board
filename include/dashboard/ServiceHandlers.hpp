#pragma once
#include "config/AppConfig.hpp"
#include "ipc/IpcClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

/// Board service management: lists the split services with their nng health +
/// s6 supervision status, and restarts an individual service via s6-svc.
class ServiceHandlers
{
public:
    explicit ServiceHandlers(const AppConfig& cfg);

    ix::HttpResponsePtr handleList(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleRestart(const ix::HttpRequestPtr& req) const;

private:
    const AppConfig& cfg_;
    // Health probes to peer services' nng REP endpoints. mutable: sendAndReceive
    // mutates the socket while the HTTP handlers stay const (matching the others).
    mutable IpcClient displayIpc_;
    mutable IpcClient wsIpc_;
    mutable IpcClient heartbeatIpc_;
    std::shared_ptr<spdlog::logger> logger_;
};

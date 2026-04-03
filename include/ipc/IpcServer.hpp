#pragma once
#include "config/AppConfig.hpp"
#include "events/EventBus.hpp"
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

class RuntimeSettings;

class IpcServer
{
public:
    IpcServer(EventBus& bus, const AppConfig& cfg, RuntimeSettings& settings);
    ~IpcServer();

    void start();
    void stop();

private:
    void _acceptLoop(const std::stop_token& st);
    void _handleClient(int clientFd, const std::stop_token& st) const;
    void _dispatch(int clientFd, const std::string& line) const;

    EventBus& bus_;
    const AppConfig& cfg_;
    RuntimeSettings& settings_;
    std::shared_ptr<spdlog::logger> logger_;

    int serverFd_ = -1;
    std::jthread acceptThread_;
    std::vector<std::jthread> clientThreads_;
    std::mutex clientMtx_;
};

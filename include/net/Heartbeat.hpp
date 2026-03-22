#pragma once
#include "config/AppConfig.hpp"
#include "events/EventBus.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <thread>

class Heartbeat
{
public:
    Heartbeat(EventBus& bus, const AppConfig& cfg);

    void start();
    void stop();

private:
    void _run(std::stop_token st);
    void _sendHeartbeat();

    EventBus& bus_;
    const AppConfig& cfg_;
    std::shared_ptr<spdlog::logger> logger_;
    std::jthread thread_;
    std::condition_variable_any cv_;
    std::mutex mtx_;
};

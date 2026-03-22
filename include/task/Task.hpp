#pragma once
#include "events/EventBus.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

class Task
{
public:
    Task(EventBus& bus, std::string name);
    virtual ~Task();

    virtual void start();
    void stop();

protected:
    virtual void _run(std::stop_token st) = 0;

    EventBus& bus_;
    std::shared_ptr<spdlog::logger> logger_;
    std::string name_;
    std::jthread thread_;
    std::mutex mtx_;
    std::condition_variable_any cv_;
};

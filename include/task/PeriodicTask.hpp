#pragma once
#include "config/AppConfig.hpp"
#include "events/EventBus.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <thread>

class PeriodicTask
{
public:
    PeriodicTask(EventBus& bus, const AppConfig& cfg, std::string name);
    virtual ~PeriodicTask();

    void start();
    void stop();

protected:
    virtual int intervalSecs() const = 0;
    virtual void execute() = 0;

    EventBus& bus_;
    const AppConfig& cfg_;
    std::shared_ptr<spdlog::logger> logger_;

private:
    void _run(std::stop_token st);

    std::string name_;
    std::jthread thread_;
    std::condition_variable_any cv_;
    std::mutex mtx_;
};

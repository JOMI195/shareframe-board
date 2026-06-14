#pragma once
#include "config/AppConfig.hpp"
#include "task/Task.hpp"

class PeriodicTask : public Task
{
public:
    PeriodicTask(EventBus& bus, const AppConfig& cfg, std::string name);

    void start() override;

    // Wake the loop to run execute() now, without waiting for the next interval.
    void runNow();

protected:
    [[nodiscard]] virtual int intervalSecs() const = 0;
    virtual void execute() = 0;

    const AppConfig& cfg_;

protected:
    void _run(std::stop_token st) override;

private:
    bool runNow_ = false;
};

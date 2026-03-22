#pragma once
#include "config/AppConfig.hpp"
#include "task/Task.hpp"

class PeriodicTask : public Task
{
public:
    PeriodicTask(EventBus& bus, const AppConfig& cfg, std::string name);

    void start() override;

protected:
    [[nodiscard]] virtual int intervalSecs() const = 0;
    virtual void execute() = 0;

    const AppConfig& cfg_;

private:
    void _run(std::stop_token st) override;
};

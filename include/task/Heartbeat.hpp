#pragma once
#include "task/PeriodicTask.hpp"

class Heartbeat : public PeriodicTask
{
public:
    Heartbeat(EventBus& bus, const AppConfig& cfg);

protected:
    int intervalSecs() const override;
    void execute() override;
};

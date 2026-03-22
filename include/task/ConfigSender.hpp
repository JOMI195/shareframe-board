#pragma once
#include "task/PeriodicTask.hpp"
#include <string>

class ConfigSender : public PeriodicTask
{
public:
    ConfigSender(EventBus& bus, const AppConfig& cfg);

protected:
    [[nodiscard]] int intervalSecs() const override;
    void execute() override;

private:
    static std::string _getLocalIp();
};

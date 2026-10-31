#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"

/// Shows the register-this-frame screen. Started and stopped by DisplayTaskManager;
/// repaints when the panel was wiped meanwhile.
class RegistrationScreen : public Task
{
public:
    RegistrationScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

protected:
    void _run(std::stop_token st) override;

private:
    const AppConfig& cfg_;
    DisplayManager& display_;
};

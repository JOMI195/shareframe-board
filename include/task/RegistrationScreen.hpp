#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"
#include <string>

/// Shows the register-this-frame screen. Started and stopped by DisplayTaskManager;
/// repaints when the claim code changes or the panel was wiped meanwhile.
class RegistrationScreen : public Task
{
public:
    RegistrationScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

    void setClaimCode(std::string claimCode);

protected:
    void _run(std::stop_token st) override;

private:
    const AppConfig& cfg_;
    DisplayManager& display_;
    std::string claimCode_;
    bool dirty_ = false;
};

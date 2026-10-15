#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"
#include <string>

/// Shows the Wi-Fi setup screen while the frame hosts its access point. Started and
/// stopped by DisplayTaskManager; repaints when the AP credentials change or the
/// panel was wiped meanwhile.
class ApNetworkSetupScreen : public Task
{
public:
    ApNetworkSetupScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

    void setCredentials(std::string ssid, std::string password);

protected:
    void _run(std::stop_token st) override;

private:
    const AppConfig& cfg_;
    DisplayManager& display_;
    std::string ssid_;
    std::string password_;
    bool dirty_ = false;
};

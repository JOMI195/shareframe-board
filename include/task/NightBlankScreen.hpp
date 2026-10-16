#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"
#include <string>
#include <vector>

/// Shows the logo with a night-mode note through the quiet-hours window, so the
/// paused frame doesn't look dead. Started and stopped by DisplayTaskManager.
class NightBlankScreen : public Task
{
public:
    NightBlankScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

    /// Call while stopped.
    void set(int endHour);

    static constexpr auto SOURCE = "night";

protected:
    void _run(std::stop_token st) override;

private:
    const AppConfig& cfg_;
    DisplayManager& display_;
    std::vector<uint8_t> buffer_;
};

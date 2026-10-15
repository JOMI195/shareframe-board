#pragma once
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"

/// Keeps the panel white through the quiet-hours window. Started and stopped by
/// DisplayTaskManager.
class NightBlankScreen : public Task
{
public:
    NightBlankScreen(EventBus& bus, DisplayManager& display);

protected:
    void _run(std::stop_token st) override;

private:
    DisplayManager& display_;
};

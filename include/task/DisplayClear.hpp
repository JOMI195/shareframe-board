#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "task/Task.hpp"

class DisplayClear : public Task
{
public:
    DisplayClear(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

    void start() override;

protected:
    void _run(std::stop_token st) override;

private:
    void _onClearDisplay();
    [[nodiscard]] int _secsUntilNextTarget() const;

    const AppConfig& cfg_;
    DisplayManager& display_;
    bool clearRequested_ = false;
};

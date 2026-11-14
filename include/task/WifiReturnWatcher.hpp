#pragma once
#include "task/PeriodicTask.hpp"
#include <functional>
#include <optional>
#include <string>

/// Polls the Wi-Fi daemon's mode file and calls back when the frame joins a network again.
class WifiReturnWatcher : public PeriodicTask
{
public:
    WifiReturnWatcher(EventBus& bus, const AppConfig& cfg, std::function<void()> onReturn);

protected:
    [[nodiscard]] int intervalSecs() const override { return 5; }
    void execute() override;

private:
    std::function<void()> onReturn_;
    std::optional<std::string> last_;
};

#pragma once
#include "display/ApSetupDebounce.hpp"
#include "events/Messages.hpp"
#include "task/PeriodicTask.hpp"

/// Polls the Wi-Fi daemon's mode file and publishes AP_SETUP_STATE when the verdict
/// or the AP credentials change.
class WifiModeWatcher : public PeriodicTask
{
public:
    WifiModeWatcher(EventBus& bus, const AppConfig& cfg);

protected:
    [[nodiscard]] int intervalSecs() const override { return 5; }
    void execute() override;

private:
    ApSetupDebounce debounce_;
    ApSetupStateEvent last_;
};

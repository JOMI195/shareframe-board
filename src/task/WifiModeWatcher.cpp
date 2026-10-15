#include "task/WifiModeWatcher.hpp"
#include "util/WifiMode.hpp"

WifiModeWatcher::WifiModeWatcher(EventBus& bus, const AppConfig& cfg)
    : PeriodicTask(bus, cfg, "WifiModeWatcher")
{
}

void WifiModeWatcher::execute()
{
    const auto mode = WifiMode::read(cfg_.display.wifiModePath);
    ApSetupStateEvent state;
    state.required = debounce_.update(mode.value("mode", ""));
    if (state.required)
    {
        state.ssid = mode.value("ap_ssid", "");
        state.password = mode.value("ap_password", "");
        if (state.ssid.empty())
        {
            logger_->warn("AP mode without an SSID in {}, keeping the current screen", cfg_.display.wifiModePath);
            return;
        }
    }

    if (state == last_)
        return;
    last_ = state;
    logger_->info("Wi-Fi setup screen {} (mode={})", state.required ? "required" : "not required",
                  mode.value("mode", ""));
    bus_.publish<Topic::AP_SETUP_STATE>(state);
}

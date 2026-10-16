#include "task/WifiModeWatcher.hpp"
#include "util/WifiMode.hpp"
#include <ctime>

namespace
{
int64_t uptimeSecs()
{
    timespec ts{};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return ts.tv_sec;
}
} // namespace

WifiModeWatcher::WifiModeWatcher(EventBus& bus, const AppConfig& cfg)
    : PeriodicTask(bus, cfg, "WifiModeWatcher"), debounce_(cfg.display.apSetupBootWindowSecs)
{
}

void WifiModeWatcher::execute()
{
    const auto mode = WifiMode::read(cfg_.display.wifiModePath);
    ApSetupStateEvent state;
    const int userNetworks = WifiMode::userNetworkCount(mode);
    state.required = debounce_.update(mode.value("mode", ""), userNetworks > 0, uptimeSecs());
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
    logger_->info("Wi-Fi setup screen {} (mode={}, user_networks={})", state.required ? "required" : "not required",
                  mode.value("mode", ""), userNetworks);
    bus_.publish<Topic::AP_SETUP_STATE>(state);
}

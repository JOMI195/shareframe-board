#include "task/WifiModeWatcher.hpp"
#include "util/PanelCode.hpp"
#include "util/WifiMode.hpp"
#include <ctime>
#include <filesystem>

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
    std::error_code ec;
    const bool joinedBefore = std::filesystem::exists(cfg_.display.wifiJoinedPath, ec);
    state.required = debounce_.update(mode.value("mode", ""), uptimeSecs(), joinedBefore);
    if (state.required)
    {
        state.ssid = mode.value("ap_ssid", "");
        state.password = mode.value("ap_password", "");
        if (state.ssid.empty())
        {
            logger_->warn("AP mode without an SSID in {}, keeping the current screen", cfg_.display.wifiModePath);
            return;
        }
        state.code = panel_code::readOrCreate(cfg_.display.setupCodePath);
    }

    if (state == last_)
        return;
    last_ = state;
    if (state.required && state.code.empty())
        logger_->error("No setup code at {}, the setup screen shows none", cfg_.display.setupCodePath);
    logger_->info("Wi-Fi setup screen {} (mode={})", state.required ? "required" : "not required",
                  mode.value("mode", ""));
    bus_.publish<Topic::AP_SETUP_STATE>(state);
}

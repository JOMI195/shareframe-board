#include "task/WifiReturnWatcher.hpp"
#include "util/WifiMode.hpp"

WifiReturnWatcher::WifiReturnWatcher(EventBus& bus, const AppConfig& cfg, std::function<void()> onReturn)
    : PeriodicTask(bus, cfg, "WifiReturnWatcher"), onReturn_(std::move(onReturn))
{
}

void WifiReturnWatcher::execute()
{
    auto mode = WifiMode::read(cfg_.display.wifiModePath).value("mode", "");
    const bool returned = last_ && *last_ != "connected" && mode == "connected";
    last_ = std::move(mode);
    if (!returned)
        return;
    logger_->info("Wi-Fi connected again");
    onReturn_();
}

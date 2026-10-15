#include "task/ApNetworkSetupScreen.hpp"
#include "display/ApNetworkSetupScreenRenderer.hpp"

namespace
{
constexpr auto kRecheck = std::chrono::seconds(5);
}

ApNetworkSetupScreen::ApNetworkSetupScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "ApNetworkSetupScreen"), cfg_(cfg), display_(display)
{
}

void ApNetworkSetupScreen::setCredentials(std::string ssid, std::string password)
{
    {
        std::lock_guard lk(mtx_);
        if (ssid == ssid_ && password == password_)
            return;
        ssid_ = std::move(ssid);
        password_ = std::move(password);
        dirty_ = true;
    }
    cv_.notify_one();
}

void ApNetworkSetupScreen::_run(const std::stop_token st)
{
    bool attempted = false;
    bool shown = false;
    while (!st.stop_requested())
    {
        std::string ssid, password;
        bool paint;
        {
            std::lock_guard lk(mtx_);
            paint = !shown || dirty_ || display_.isCleared();
            dirty_ = false;
            ssid = ssid_;
            password = password_;
        }

        if (paint)
        {
            // Show the QR as soon as the AP is up; later repaints respect panel wear.
            const auto policy = attempted ? RefreshPolicy::RespectMinInterval : RefreshPolicy::Immediate;
            attempted = true;
            const auto buffer = ApNetworkSetupScreenRenderer::render(ssid, password, cfg_.display.setupUrl,
                                                                     cfg_.display.setupFontPath);
            shown = display_.displayBuffer(buffer, "ap-setup", st, policy);
            if (shown)
                logger_->info("Showing Wi-Fi setup screen for AP {}", ssid);
            else if (!st.stop_requested())
                logger_->error("Failed to show Wi-Fi setup screen");
        }

        std::unique_lock lk(mtx_);
        cv_.wait_for(lk, kRecheck, [&st, this] { return st.stop_requested() || dirty_; });
    }
}

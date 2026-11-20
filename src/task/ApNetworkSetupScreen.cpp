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

void ApNetworkSetupScreen::setCredentials(std::string ssid, std::string password, std::string code,
                                          std::string failure)
{
    {
        std::lock_guard lk(mtx_);
        if (ssid == ssid_ && password == password_ && code == code_ && failure == failure_)
            return;
        failure_ = std::move(failure);
        ssid_ = std::move(ssid);
        password_ = std::move(password);
        code_ = std::move(code);
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
        std::string ssid, password, code, failure;
        bool paint;
        {
            std::lock_guard lk(mtx_);
            paint = !shown || dirty_ || display_.isCleared();
            dirty_ = false;
            ssid = ssid_;
            password = password_;
            code = code_;
            failure = failure_;
        }

        if (paint)
        {
            // Show the QR as soon as the AP is up; later repaints respect panel wear.
            const auto policy = attempted ? RefreshPolicy::RespectMinInterval : RefreshPolicy::Immediate;
            attempted = true;
            const auto text = ApNetworkSetupScreenRenderer::paragraphs(ssid, password, cfg_.display.setupUrl, code, failure);
            const auto buffer = ApNetworkSetupScreenRenderer::render(ssid, password, text, cfg_.display.setupFontPath,
                                                                     !code.empty());
            shown = display_.displayBuffer(buffer, "ap-setup", st, policy, text);
            if (shown)
                logger_->info("Showing Wi-Fi setup screen for AP {}", ssid);
            else if (!st.stop_requested())
                logger_->error("Failed to show Wi-Fi setup screen");
        }

        std::unique_lock lk(mtx_);
        cv_.wait_for(lk, kRecheck, [&st, this] { return st.stop_requested() || dirty_; });
    }
}

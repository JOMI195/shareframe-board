#include "task/RegistrationScreen.hpp"
#include "display/RegistrationScreenRenderer.hpp"

namespace
{
constexpr auto kRecheck = std::chrono::seconds(5);
}

RegistrationScreen::RegistrationScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "RegistrationScreen"), cfg_(cfg), display_(display)
{
}

void RegistrationScreen::_run(const std::stop_token st)
{
    const auto url = RegistrationScreenRenderer::registerUrl(cfg_.httpBaseUrl(), cfg_.display.registerPath,
                                                             cfg_.frameId);
    bool shown = false;
    while (!st.stop_requested())
    {
        if (!shown || display_.isCleared())
        {
            const auto buffer = RegistrationScreenRenderer::render(url, cfg_.baseUrl, cfg_.frameId,
                                                                   cfg_.display.setupFontPath);
            shown = display_.displayBuffer(buffer, "registration", st);
            if (shown)
                logger_->info("Showing registration screen for {}", cfg_.frameId);
            else if (!st.stop_requested())
                logger_->error("Failed to show registration screen");
        }

        std::unique_lock lk(mtx_);
        cv_.wait_for(lk, st, kRecheck, [] { return false; });
    }
}

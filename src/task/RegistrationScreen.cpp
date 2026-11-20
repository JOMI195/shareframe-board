#include "task/RegistrationScreen.hpp"
#include "display/RegistrationScreenRenderer.hpp"
#include <unistd.h>

namespace
{
constexpr auto kRecheck = std::chrono::seconds(5);

std::string hostname()
{
    char host[256]{};
    return gethostname(host, sizeof host - 1) == 0 ? std::string(host) : std::string{};
}
}

RegistrationScreen::RegistrationScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "RegistrationScreen"), cfg_(cfg), display_(display)
{
}

void RegistrationScreen::setClaimCode(std::string claimCode)
{
    {
        std::lock_guard lk(mtx_);
        if (claimCode == claimCode_)
            return;
        claimCode_ = std::move(claimCode);
        dirty_ = true;
    }
    cv_.notify_one();
}

void RegistrationScreen::_run(const std::stop_token st)
{
    bool shown = false;
    while (!st.stop_requested())
    {
        std::string claimCode;
        bool paint;
        {
            std::lock_guard lk(mtx_);
            paint = !shown || dirty_ || display_.isCleared();
            dirty_ = false;
            claimCode = claimCode_;
        }

        if (paint)
        {
            const auto url = RegistrationScreenRenderer::registerUrl(cfg_.httpBaseUrl(), cfg_.display.registerPath,
                                                                     claimCode);
            const auto text = RegistrationScreenRenderer::paragraphs(cfg_.baseUrl, cfg_.frameId, claimCode, hostname());
            const auto buffer = RegistrationScreenRenderer::render(url, text, cfg_.display.setupFontPath);
            shown = display_.displayBuffer(buffer, "registration", st, RefreshPolicy::RespectMinInterval, text);
            if (shown)
                logger_->info("Showing registration screen for {}", cfg_.frameId);
            else if (!st.stop_requested())
                logger_->error("Failed to show registration screen");
        }

        std::unique_lock lk(mtx_);
        cv_.wait_for(lk, st, kRecheck, [this] { return dirty_; });
    }
}

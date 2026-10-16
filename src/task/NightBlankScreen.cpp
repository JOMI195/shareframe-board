#include "task/NightBlankScreen.hpp"
#include "display/LogoScreenRenderer.hpp"
#include <filesystem>

NightBlankScreen::NightBlankScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "NightBlankScreen"), cfg_(cfg), display_(display)
{
}

void NightBlankScreen::set(const int endHour)
{
    const auto image = std::filesystem::path(cfg_.display.loadingImagePath) / LogoScreenRenderer::IMAGE;
    auto buffer = LogoScreenRenderer::render(image, "Nachtmodus aktiv",
                                             "Bilder wieder ab " + std::to_string(endHour) + " Uhr",
                                             cfg_.display.boldFontPath, cfg_.display.setupFontPath);
    std::lock_guard lk(mtx_);
    buffer_ = std::move(buffer);
}

void NightBlankScreen::_run(const std::stop_token st)
{
    std::vector<uint8_t> buffer;
    {
        std::lock_guard lk(mtx_);
        buffer = buffer_;
    }

    if (display_.displayBuffer(buffer, SOURCE, st))
        logger_->info("Showing the night screen");
    else if (!st.stop_requested())
        logger_->error("Failed to show the night screen");

    std::unique_lock lk(mtx_);
    cv_.wait(lk, st, [] { return false; });
}

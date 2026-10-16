#include "task/StatusScreen.hpp"
#include "display/LogoScreenRenderer.hpp"
#include <filesystem>

StatusScreen::StatusScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "StatusScreen"), cfg_(cfg), display_(display)
{
}

void StatusScreen::set(const StatusScreenKind kind, const std::string& version)
{
    std::string title, detail;
    switch (kind)
    {
    case StatusScreenKind::Update:
        title = "Update wird installiert …";
        detail = version.empty() ? "" : "v" + version;
        break;
    case StatusScreenKind::Reboot:
        title = "Neustart …";
        break;
    case StatusScreenKind::FactoryReset:
        title = "Wird zurückgesetzt …";
        detail = "Werkseinstellungen";
        break;
    }
    const auto image = std::filesystem::path(cfg_.display.loadingImagePath) / LogoScreenRenderer::IMAGE;
    std::lock_guard lk(mtx_);
    buffer_ = LogoScreenRenderer::render(image, title, detail, cfg_.display.boldFontPath, cfg_.display.setupFontPath);
    source_ = "status:" + toString(kind);
    shown_ = false;
}

void StatusScreen::finishForShutdown()
{
    std::lock_guard lk(mtx_);
    if (!shown_ && !buffer_.empty())
        shown_ = display_.paintForShutdown(buffer_, source_);
}

void StatusScreen::_run(const std::stop_token st)
{
    std::vector<uint8_t> buffer;
    std::string source;
    {
        std::lock_guard lk(mtx_);
        if (shown_)
            return;
        buffer = buffer_;
        source = source_;
    }

    const bool shown = display_.displayBuffer(buffer, source, st);
    if (shown)
        logger_->info("Showing {}", source);
    else if (!st.stop_requested())
        logger_->error("Failed to show {}", source);

    std::unique_lock lk(mtx_);
    shown_ = shown;
    cv_.wait(lk, st, [] { return false; });
}

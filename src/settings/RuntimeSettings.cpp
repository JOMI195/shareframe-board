#include "settings/RuntimeSettings.hpp"

RuntimeSettings::RuntimeSettings(SettingsRepository& repo, const AppConfig& cfg)
    : repo_(repo),
      logger_(spdlog::default_logger()->clone("RuntimeSettings")),
      displayIntervalSecs_(cfg.display.intervalSecs)
{
    // Restore persisted value from DB if available
    auto stored = repo_.get("display_interval");
    if (stored)
    {
        try
        {
            int val = std::stoi(*stored);
            if (val > 0)
            {
                displayIntervalSecs_ = val;
                logger_->info("Restored display interval from DB: {}s", val);
            }
        }
        catch (...) {}
    }

    // Restore slideshow active state
    auto storedActive = repo_.get("slideshow_active");
    if (storedActive)
    {
        slideshowActive_ = (*storedActive == "1");
        logger_->info("Restored slideshow active from DB: {}", slideshowActive_);
    }
}

int RuntimeSettings::getDisplayInterval() const
{
    std::lock_guard lk(mtx_);
    return displayIntervalSecs_;
}

void RuntimeSettings::setDisplayInterval(int secs)
{
    std::lock_guard lk(mtx_);
    displayIntervalSecs_ = secs;
    repo_.set("display_interval", std::to_string(secs));
    logger_->info("Display interval updated to {}s (persisted)", secs);
}

bool RuntimeSettings::isSlideshowActive() const
{
    std::lock_guard lk(mtx_);
    return slideshowActive_;
}

void RuntimeSettings::setSlideshowActive(bool active)
{
    std::lock_guard lk(mtx_);
    slideshowActive_ = active;
    repo_.set("slideshow_active", active ? "1" : "0");
    logger_->info("Slideshow active updated to {} (persisted)", active);
}

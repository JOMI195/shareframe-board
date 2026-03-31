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

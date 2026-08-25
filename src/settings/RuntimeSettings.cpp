#include "settings/RuntimeSettings.hpp"
#include <algorithm>
#include <ctime>

RuntimeSettings::RuntimeSettings(SettingsRepository& repo, const AppConfig& cfg)
    : repo_(repo),
      logger_(spdlog::default_logger()->clone("RuntimeSettings")),
      displayIntervalSecs_(cfg.display.intervalSecs),
      nightModeEnabled_(cfg.display.nightModeEnabled),
      nightStartHour_(cfg.display.nightStartHour),
      nightEndHour_(cfg.display.nightEndHour),
      nightIntervalSecs_(cfg.display.nightIntervalSecs)
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

    // Restore night mode
    if (auto storedNight = repo_.get("night_mode_enabled"))
        nightModeEnabled_ = (*storedNight == "1");

    const auto restoreHour = [this](const char* key, int& target)
    {
        auto storedHour = repo_.get(key);
        if (!storedHour)
            return;
        try
        {
            if (const int val = std::stoi(*storedHour); val >= 0 && val <= 23)
                target = val;
        }
        catch (...) {}
    };
    restoreHour("night_start_hour", nightStartHour_);
    restoreHour("night_end_hour", nightEndHour_);

    if (auto storedNightInterval = repo_.get("night_interval"))
    {
        try
        {
            if (const int val = std::stoi(*storedNightInterval); val > 0)
                nightIntervalSecs_ = val;
        }
        catch (...) {}
    }

    logger_->info("Night mode: enabled={}, window={}-{}, interval={}s",
                  nightModeEnabled_, nightStartHour_, nightEndHour_, nightIntervalSecs_);
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

bool RuntimeSettings::isNightModeEnabled() const
{
    std::lock_guard lk(mtx_);
    return nightModeEnabled_;
}

int RuntimeSettings::getNightStartHour() const
{
    std::lock_guard lk(mtx_);
    return nightStartHour_;
}

int RuntimeSettings::getNightEndHour() const
{
    std::lock_guard lk(mtx_);
    return nightEndHour_;
}

int RuntimeSettings::getNightInterval() const
{
    std::lock_guard lk(mtx_);
    return nightIntervalSecs_;
}

void RuntimeSettings::setNightMode(const bool enabled, const int startHour,
                                   const int endHour, const int intervalSecs)
{
    std::lock_guard lk(mtx_);
    nightModeEnabled_ = enabled;
    nightStartHour_ = startHour;
    nightEndHour_ = endHour;
    nightIntervalSecs_ = intervalSecs;
    repo_.set("night_mode_enabled", enabled ? "1" : "0");
    repo_.set("night_start_hour", std::to_string(startHour));
    repo_.set("night_end_hour", std::to_string(endHour));
    repo_.set("night_interval", std::to_string(intervalSecs));
    logger_->info("Night mode updated to enabled={}, window={}-{}, interval={}s (persisted)",
                  enabled, startHour, endHour, intervalSecs);
}

bool RuntimeSettings::isNightNow() const
{
    std::lock_guard lk(mtx_);
    return nightModeEnabled_ && _secsUntilNightEnd() > 0;
}

int RuntimeSettings::getEffectiveInterval() const
{
    std::lock_guard lk(mtx_);
    if (!nightModeEnabled_)
        return displayIntervalSecs_;

    const int untilEnd = _secsUntilNightEnd();
    if (untilEnd <= 0)
        return displayIntervalSecs_;

    // Clamping to the window end makes the loop resume on time instead of
    // drifting past it, without costing an extra refresh.
    return std::min(nightIntervalSecs_, untilEnd);
}

int RuntimeSettings::_secsUntilNightEnd() const
{
    const std::time_t nowT = std::time(nullptr);
    std::tm local{};
    localtime_r(&nowT, &local);

    const int hour = local.tm_hour;
    // The window is half-open and may wrap past midnight.
    const bool inside = (nightStartHour_ < nightEndHour_)
        ? (hour >= nightStartHour_ && hour < nightEndHour_)
        : (hour >= nightStartHour_ || hour < nightEndHour_);
    if (!inside)
        return 0;

    std::tm end = local;
    end.tm_hour = nightEndHour_;
    end.tm_min = 0;
    end.tm_sec = 0;

    std::time_t endT = std::mktime(&end);
    if (endT <= nowT)
        endT += 24 * 60 * 60;

    return static_cast<int>(endT - nowT);
}

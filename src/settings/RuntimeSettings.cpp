#include "settings/RuntimeSettings.hpp"
#include "settings/NightWindow.hpp"
#include <algorithm>
#include <ctime>
#include <nlohmann/json.hpp>

namespace
{
std::tm localNow()
{
    const std::time_t nowT = std::time(nullptr);
    std::tm local{};
    localtime_r(&nowT, &local);
    return local;
}
} // namespace

RuntimeSettings::RuntimeSettings(SettingsRepository& repo, const AppConfig& cfg)
    : repo_(repo),
      logger_(spdlog::default_logger()->clone("RuntimeSettings")),
      displayIntervalSecs_(cfg.display.intervalSecs),
      nightModeEnabled_(cfg.display.nightModeEnabled),
      nightStartHour_(cfg.display.nightStartHour),
      nightEndHour_(cfg.display.nightEndHour),
      nightIntervalSecs_(cfg.display.nightIntervalSecs),
      nightStyle_(nightStyleFromString(cfg.display.nightStyle).value_or(NightStyle::Hold))
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

    if (auto storedStyle = repo_.get("night_style"))
    {
        if (const auto style = nightStyleFromString(*storedStyle))
            nightStyle_ = *style;
    }

    logger_->info("Night mode: enabled={}, window={}-{}, interval={}s, style={}",
                  nightModeEnabled_, nightStartHour_, nightEndHour_, nightIntervalSecs_, toString(nightStyle_));

    if (auto storedMode = repo_.get("playback_mode"))
    {
        if (const auto mode = playbackModeFromString(*storedMode))
            playbackMode_ = *mode;
    }
    if (auto storedOrder = repo_.get("playback_custom_order"))
    {
        try { customOrder_ = nlohmann::json::parse(*storedOrder).get<std::vector<int64_t>>(); }
        catch (...) {}
    }
    logger_->info("Playback order: {} ({} custom entries)", toString(playbackMode_), customOrder_.size());
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

NightStyle RuntimeSettings::getNightStyle() const
{
    std::lock_guard lk(mtx_);
    return nightStyle_;
}

void RuntimeSettings::setNightMode(const bool enabled, const int startHour,
                                   const int endHour, const int intervalSecs, const NightStyle style)
{
    std::lock_guard lk(mtx_);
    nightModeEnabled_ = enabled;
    nightStartHour_ = startHour;
    nightEndHour_ = endHour;
    nightIntervalSecs_ = intervalSecs;
    nightStyle_ = style;
    repo_.set("night_mode_enabled", enabled ? "1" : "0");
    repo_.set("night_start_hour", std::to_string(startHour));
    repo_.set("night_end_hour", std::to_string(endHour));
    repo_.set("night_interval", std::to_string(intervalSecs));
    repo_.set("night_style", toString(style));
    logger_->info("Night mode updated to enabled={}, window={}-{}, interval={}s, style={} (persisted)",
                  enabled, startHour, endHour, intervalSecs, toString(style));
}

PlaybackMode RuntimeSettings::getPlaybackMode() const
{
    std::lock_guard lk(mtx_);
    return playbackMode_;
}

std::vector<int64_t> RuntimeSettings::getCustomOrder() const
{
    std::lock_guard lk(mtx_);
    return customOrder_;
}

void RuntimeSettings::setPlaybackOrder(const PlaybackMode mode, const std::vector<int64_t>& customOrder)
{
    std::lock_guard lk(mtx_);
    playbackMode_ = mode;
    customOrder_ = customOrder;
    repo_.set("playback_mode", toString(mode));
    repo_.set("playback_custom_order", nlohmann::json(customOrder).dump());
    logger_->info("Playback order updated to {} ({} custom entries, persisted)", toString(mode), customOrder.size());
}

bool RuntimeSettings::isNightNow() const
{
    std::lock_guard lk(mtx_);
    return nightModeEnabled_ && _secsUntilNightEnd() > 0;
}

bool RuntimeSettings::isNightBlankNow() const
{
    std::lock_guard lk(mtx_);
    return nightModeEnabled_ && nightStyle_ == NightStyle::Pause && _secsUntilNightEnd() > 0;
}

bool RuntimeSettings::isNightHoldNow() const
{
    std::lock_guard lk(mtx_);
    return nightModeEnabled_ && nightStyle_ == NightStyle::Hold && _secsUntilNightEnd() > 0;
}

int RuntimeSettings::secsUntilNightBlankChange() const
{
    std::lock_guard lk(mtx_);
    if (!nightModeEnabled_ || nightStyle_ != NightStyle::Pause || nightStartHour_ == nightEndHour_)
        return -1;
    if (const int untilEnd = _secsUntilNightEnd(); untilEnd > 0)
        return untilEnd;
    return _secsUntilNightStart();
}

int RuntimeSettings::getEffectiveInterval() const
{
    std::lock_guard lk(mtx_);
    if (!nightModeEnabled_)
        return displayIntervalSecs_;

    const int untilEnd = _secsUntilNightEnd();
    if (untilEnd <= 0)
        return displayIntervalSecs_;
    if (nightStyle_ == NightStyle::Hold)
        return untilEnd;

    // Clamping to the window end makes the loop resume on time instead of
    // drifting past it, without costing an extra refresh.
    return std::min(nightIntervalSecs_, untilEnd);
}

int RuntimeSettings::_secsUntilNightEnd() const
{
    return night::secsUntilEnd(localNow(), nightStartHour_, nightEndHour_);
}

int RuntimeSettings::_secsUntilNightStart() const
{
    return night::secsUntilStart(localNow(), nightStartHour_, nightEndHour_);
}

#pragma once
#include "config/AppConfig.hpp"
#include "repository/SettingsRepository.hpp"
#include <mutex>
#include <spdlog/spdlog.h>

class RuntimeSettings
{
public:
    RuntimeSettings(SettingsRepository& repo, const AppConfig& cfg);

    [[nodiscard]] int getDisplayInterval() const;
    void setDisplayInterval(int secs);

    [[nodiscard]] bool isSlideshowActive() const;
    void setSlideshowActive(bool active);

    [[nodiscard]] bool isNightModeEnabled() const;
    [[nodiscard]] int getNightStartHour() const;
    [[nodiscard]] int getNightEndHour() const;
    [[nodiscard]] int getNightInterval() const;
    void setNightMode(bool enabled, int startHour, int endHour, int intervalSecs);

    /// True while the wall clock sits inside an enabled quiet-hours window.
    [[nodiscard]] bool isNightNow() const;

    /// Interval to use right now: the night one clamped to the end of the
    /// window while inside it, the day one otherwise.
    [[nodiscard]] int getEffectiveInterval() const;

private:
    /// Seconds left in the quiet-hours window, 0 when outside it. Call under lock.
    [[nodiscard]] int _secsUntilNightEnd() const;

    SettingsRepository& repo_;
    std::shared_ptr<spdlog::logger> logger_;
    mutable std::mutex mtx_;
    int displayIntervalSecs_;
    bool slideshowActive_ = true;
    bool nightModeEnabled_;
    int nightStartHour_;
    int nightEndHour_;
    int nightIntervalSecs_;
};

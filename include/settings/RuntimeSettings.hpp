#pragma once
#include "config/AppConfig.hpp"
#include "settings/NightWindow.hpp"
#include "settings/PlaybackOrder.hpp"
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
    [[nodiscard]] NightStyle getNightStyle() const;
    void setNightMode(bool enabled, int startHour, int endHour, int intervalSecs, NightStyle style);

    [[nodiscard]] PlaybackMode getPlaybackMode() const;
    [[nodiscard]] std::vector<int64_t> getCustomOrder() const;
    void setPlaybackOrder(PlaybackMode mode, const std::vector<int64_t>& customOrder);

    [[nodiscard]] bool isShowSender() const;
    void setShowSender(bool show);

    /// Day unless the wall clock sits inside an enabled quiet-hours window.
    [[nodiscard]] NightState nightState() const;

    /// Seconds until the panel next pauses for the night or resumes, -1 when pausing is off.
    [[nodiscard]] int secsUntilNightBlankChange() const;

    /// Interval to use right now: the night one clamped to the end of the
    /// window while inside it (the whole rest of it when holding), the day one otherwise.
    [[nodiscard]] int getEffectiveInterval() const;

private:
    /// Call under lock.
    [[nodiscard]] NightState _nightState() const;
    /// Seconds left in the quiet-hours window, 0 when outside it. Call under lock.
    [[nodiscard]] int _secsUntilNightEnd() const;
    /// Seconds until the quiet-hours window opens, 0 when inside it. Call under lock.
    [[nodiscard]] int _secsUntilNightStart() const;

    SettingsRepository& repo_;
    std::shared_ptr<spdlog::logger> logger_;
    mutable std::mutex mtx_;
    int displayIntervalSecs_;
    bool slideshowActive_ = true;
    bool nightModeEnabled_;
    int nightStartHour_;
    int nightEndHour_;
    int nightIntervalSecs_;
    NightStyle nightStyle_;
    PlaybackMode playbackMode_ = PlaybackMode::Chronological;
    std::vector<int64_t> customOrder_;
    bool showSender_ = false;
};

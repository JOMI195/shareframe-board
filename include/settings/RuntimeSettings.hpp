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

private:
    SettingsRepository& repo_;
    std::shared_ptr<spdlog::logger> logger_;
    mutable std::mutex mtx_;
    int displayIntervalSecs_;
    bool slideshowActive_ = true;
};

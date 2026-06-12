#pragma once
#include "display/DisplayManager.hpp"
#include "repository/ImageRepository.hpp"
#include "task/Task.hpp"
#include <atomic>
#include <filesystem>
#include <vector>

class RuntimeSettings;
struct SetSlideshowActiveEvent;

class DisplayImageLoop : public Task
{
public:
    DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                     ImageRepository& repo, DisplayManager& display,
                     RuntimeSettings& settings);

    void start() override;

    /// Seconds until the next scheduled image change, or -1 when the slideshow
    /// is paused / no change is scheduled. Thread-safe (atomic read).
    [[nodiscard]] int secondsUntilNext() const;

    /// True once the loop has displayed its first image (default or user).
    [[nodiscard]] bool isReady() const { return ready_.load(); }

protected:
    void _run(std::stop_token st) override;

private:
    void _armNextChange(); // schedule next change = now + interval
    void _onImageRemoved(const std::vector<int64_t>& removedIds);
    void _onSkipImage();
    void _onUpdateInterval(const UpdateDisplayIntervalEvent& evt) const;
    void _onSetSlideshowActive(const SetSlideshowActiveEvent& evt);
    [[nodiscard]] std::vector<std::filesystem::path> _loadDefaultImages() const;

    const AppConfig& cfg_;
    ImageRepository& repo_;
    DisplayManager& display_;
    RuntimeSettings& settings_;
    int64_t currentImageId_ = 0;
    bool skipCurrent_ = false;
    size_t defaultImageIdx_ = 0;
    // steady_clock ms at which the next image change is due; 0 = none scheduled.
    std::atomic<int64_t> nextChangeAtMs_{0};
    std::atomic<bool> ready_{false};
};

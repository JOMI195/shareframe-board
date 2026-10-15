#pragma once
#include "display/DisplayManager.hpp"
#include "repository/ImageRepository.hpp"
#include "settings/PlaybackOrder.hpp"
#include "task/Task.hpp"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <vector>

class RuntimeSettings;
struct SetSlideshowActiveEvent;
struct UpdateNightModeEvent;
struct UpdatePlaybackOrderEvent;

class DisplayImageLoop : public Task
{
public:
    DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                     ImageRepository& repo, DisplayManager& display,
                     RuntimeSettings& settings);

    /// Bus handlers stay registered across stop/start; call once before the bus starts.
    void subscribe();

    void start() override;

    /// Seconds until the next scheduled image change, or -1 when the slideshow
    /// is paused / no change is scheduled. Thread-safe (atomic read).
    [[nodiscard]] int secondsUntilNext() const;

    /// True once the loop has displayed its first image (default or user).
    [[nodiscard]] bool isReady() const { return ready_.load(); }

    /// True while the loop paints; a skip arriving then would be lost.
    [[nodiscard]] bool isRefreshing() const { return refreshing_.load(); }

    /// Id of the user picture shown last, 0 while none was shown.
    [[nodiscard]] int64_t currentImageId() const { return currentImageId_.load(); }

    /// The current shuffle cycle, in play order.
    [[nodiscard]] std::vector<int64_t> shuffleOrder() const;

protected:
    void _run(std::stop_token st) override;

private:
    void _armNextChange(); // schedule next change = now + interval
    [[nodiscard]] bool _paint(const std::filesystem::path& path, const std::stop_token& st) const;
    void _onImageRemoved(const std::vector<int64_t>& removedIds);
    void _onImageNew();
    void _onSkipImage();
    void _onUpdateInterval(const UpdateDisplayIntervalEvent& evt) const;
    void _onUpdateNightMode(const UpdateNightModeEvent& evt) const;
    void _onUpdatePlaybackOrder(const UpdatePlaybackOrderEvent& evt) const;
    [[nodiscard]] int64_t _pickNext(const std::vector<Image>& images);
    void _onSetSlideshowActive(const SetSlideshowActiveEvent& evt);
    [[nodiscard]] std::vector<std::filesystem::path> _loadDefaultImages() const;

    const AppConfig& cfg_;
    ImageRepository& repo_;
    DisplayManager& display_;
    RuntimeSettings& settings_;
    std::atomic<int64_t> currentImageId_{0};
    mutable std::mutex shuffleMtx_;
    playback::ShuffleBag shuffle_;
    bool skipCurrent_ = false;
    bool newImage_ = false;
    size_t defaultImageIdx_ = 0;
    // steady_clock ms at which the next image change is due; 0 = none scheduled.
    std::atomic<int64_t> nextChangeAtMs_{0};
    std::atomic<bool> ready_{false};
    std::atomic<bool> refreshing_{false};
};

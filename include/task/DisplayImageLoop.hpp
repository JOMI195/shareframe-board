#pragma once
#include "display/DisplayManager.hpp"
#include "display/PanelText.hpp"
#include "display/Slideshow.hpp"
#include "fsm/Machine.hpp"
#include "repository/ImageRepository.hpp"
#include "settings/PlaybackOrder.hpp"
#include "task/Task.hpp"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <random>
#include <vector>

class RuntimeSettings;
struct SetSlideshowActiveEvent;
struct ShowImageEvent;
struct UpdateCustomizationEvent;
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
    [[nodiscard]] bool isRefreshing() const { return phase_.state() == slideshow::Phase::Painting; }

    /// Id of the user picture shown last, 0 while none was shown.
    [[nodiscard]] int64_t currentImageId() const { return currentImageId_.load(); }

    /// The current shuffle cycle, in play order.
    [[nodiscard]] std::vector<int64_t> shuffleOrder() const;

protected:
    void _run(std::stop_token st) override;

private:
    /// What a paint put on the panel, kept so a wiped panel can get it back.
    struct Frame
    {
        slideshow::Content content;
        std::filesystem::path path;
        /// Empty never gets a badge.
        std::string sender;
    };

    /// One pass of the loop: looks at what there is to show and runs the matching phase.
    void _step(const std::stop_token& st);
    void _pause(const std::stop_token& st);
    void _waitForContent(const std::stop_token& st);
    void _showSample(const std::stop_token& st, const std::vector<std::filesystem::path>& samples);
    void _showPicture(const std::stop_token& st, const std::vector<Image>& pictures);
    void _recover(const std::stop_token& st);
    void _armNextChange(); // schedule next change = now + interval
    /// Paints the frame with its badges. A sample gets the no-images note at a new spot each time.
    [[nodiscard]] bool _paint(const Frame& frame, const std::stop_token& st);
    /// Keeps the frame up until the armed change is due or something ends its turn. Repaints
    /// it once the refresh interval allows if the daily clear wiped it before then.
    void _hold(const std::stop_token& st, const Frame& frame);
    /// Call under lock.
    [[nodiscard]] bool _endsHold(slideshow::Content held) const;
    void _onScheduledClear();
    void _onImageRemoved(const std::vector<int64_t>& removedIds);
    void _onImageNew();
    void _onSkipImage();
    void _onShowImage(const ShowImageEvent& evt);
    /// The id asked for via show-now if it is still in `images`, else the regular next one.
    [[nodiscard]] int64_t _takeNext(const std::vector<Image>& images);
    void _onUpdateInterval(const UpdateDisplayIntervalEvent& evt) const;
    void _onUpdateNightMode(const UpdateNightModeEvent& evt) const;
    void _onUpdatePlaybackOrder(const UpdatePlaybackOrderEvent& evt) const;
    void _onUpdateCustomization(const UpdateCustomizationEvent& evt) const;
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
    slideshow::Requests requests_;
    size_t defaultImageIdx_ = 0;
    std::mt19937 noteRng_{std::random_device{}()};
    std::optional<panel_text::Rect> lastNote_;
    // steady_clock ms at which the next image change is due; 0 = none scheduled.
    std::atomic<int64_t> nextChangeAtMs_{0};
    std::atomic<bool> ready_{false};
    fsm::Machine<slideshow::Phase> phase_;
};

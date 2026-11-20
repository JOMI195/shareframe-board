#include "task/DisplayImageLoop.hpp"
#include "display/PanelBadge.hpp"
#include "display/PhotoRenderer.hpp"
#include "events/Messages.hpp"
#include "settings/RuntimeSettings.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>

namespace
{
using slideshow::Content;
using slideshow::Phase;

int64_t steadyNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace

DisplayImageLoop::DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                                   ImageRepository& repo, DisplayManager& display,
                                   RuntimeSettings& settings)
    : Task(bus, "DisplayImageLoop"), cfg_(cfg), repo_(repo), display_(display), settings_(settings),
      phase_(Phase::Stopped, logger_, spdlog::level::debug)
{
}

void DisplayImageLoop::subscribe()
{
    bus_.subscribe<Topic::IMAGE_REMOVED>(
        [this](const std::vector<int64_t>& ids) { _onImageRemoved(ids); });
    bus_.subscribe<Topic::IMAGE_NEW>(
        [this](const int64_t&) { _onImageNew(); });
    bus_.subscribe<Topic::SKIP_IMAGE>(
        [this](const SkipImageEvent&) { _onSkipImage(); });
    bus_.subscribe<Topic::SHOW_IMAGE>(
        [this](const ShowImageEvent& evt) { _onShowImage(evt); });
    bus_.subscribe<Topic::UPDATE_DISPLAY_INTERVAL>(
        [this](const UpdateDisplayIntervalEvent& evt) { _onUpdateInterval(evt); });
    bus_.subscribe<Topic::UPDATE_NIGHT_MODE>(
        [this](const UpdateNightModeEvent& evt) { _onUpdateNightMode(evt); });
    bus_.subscribe<Topic::UPDATE_PLAYBACK_ORDER>(
        [this](const UpdatePlaybackOrderEvent& evt) { _onUpdatePlaybackOrder(evt); });
    bus_.subscribe<Topic::SET_SLIDESHOW_ACTIVE>(
        [this](const SetSlideshowActiveEvent& evt) { _onSetSlideshowActive(evt); });
    bus_.subscribe<Topic::UPDATE_CUSTOMIZATION>(
        [this](const UpdateCustomizationEvent& evt) { _onUpdateCustomization(evt); });
    bus_.subscribe<Topic::SCHEDULED_CLEAR>(
        [this](const ScheduledClearEvent&) { _onScheduledClear(); });
}

void DisplayImageLoop::start()
{
    logger_->info("Starting DisplayImageLoop (interval={}s, minRefresh={}s, active={})",
                  settings_.getDisplayInterval(), cfg_.display.minRefreshSecs,
                  settings_.isSlideshowActive());
    logger_->info("Night mode {} (window={}-{}, interval={}s)",
                  settings_.isNightModeEnabled() ? "enabled" : "disabled",
                  settings_.getNightStartHour(), settings_.getNightEndHour(),
                  settings_.getNightInterval());
    Task::start();
}

void DisplayImageLoop::_armNextChange()
{
    nextChangeAtMs_.store(steadyNowMs()
        + static_cast<int64_t>(settings_.getEffectiveInterval()) * 1000);
}

int DisplayImageLoop::secondsUntilNext() const
{
    if (!settings_.isSlideshowActive())
        return -1;
    const int64_t at = nextChangeAtMs_.load();
    if (at == 0)
        return -1;
    const int64_t remMs = at - steadyNowMs();
    if (remMs <= 0)
        return 0;
    int secs = static_cast<int>(remMs / 1000);
    // Guard against a stale arm reporting more than a full interval.
    if (const int interval = settings_.getEffectiveInterval(); secs > interval)
        secs = interval;
    return secs;
}

void DisplayImageLoop::_onImageRemoved(const std::vector<int64_t>& removedIds)
{
    // Removed mid-paint: stays on the panel until the next change, by design.
    std::lock_guard lk(mtx_);
    if (const int64_t current = currentImageId_.load(); std::ranges::find(removedIds, current) != removedIds.end())
    {
        logger_->info("Current image {} was removed, skipping", current);
        requests_.skip = true;
        cv_.notify_one();
    }
}

std::vector<std::filesystem::path> DisplayImageLoop::_loadDefaultImages() const
{
    std::vector<std::filesystem::path> paths;
    const auto& dir = cfg_.display.defaultImagesPath;

    if (!std::filesystem::exists(dir))
    {
        logger_->warn("Default images directory not found: {}", dir);
        return paths;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.is_regular_file())
            paths.push_back(entry.path());
    }

    std::ranges::sort(paths);
    logger_->debug("Loaded {} default images from {}", paths.size(), dir);
    return paths;
}

std::vector<Image> DisplayImageLoop::_showablePictures() const
{
    auto pictures = repo_.getAll();
    std::erase_if(pictures, [this](const Image& image)
    {
        if (unreadable_.contains(image.id))
            return true;
        if (std::filesystem::exists(image.imagePath))
            return false;
        logger_->warn("Image file missing: {}, skipping", image.imagePath.string());
        return true;
    });
    return pictures;
}

void DisplayImageLoop::_run(const std::stop_token st)
{
    while (!st.stop_requested())
    {
        try
        {
            _step(st);
        }
        catch (const std::exception& e)
        {
            logger_->error("DisplayImageLoop iteration failed: {}", e.what());
            _recover(st);
        }
        catch (...)
        {
            logger_->error("DisplayImageLoop iteration failed with unknown exception");
            _recover(st);
        }
    }
    phase_.moveTo(Phase::Stopped);
    nextChangeAtMs_.store(0);
}

void DisplayImageLoop::_step(const std::stop_token& st)
{
    const bool active = settings_.isSlideshowActive();
    const auto pictures = active ? _showablePictures() : std::vector<Image>{};
    const auto samples = active && pictures.empty() ? _loadDefaultImages() : std::vector<std::filesystem::path>{};

    switch (slideshow::plan(active, !pictures.empty(), !samples.empty()))
    {
    case slideshow::Plan::Pause: return _pause(st);
    case slideshow::Plan::WaitForContent: return _waitForContent(st);
    case slideshow::Plan::ShowSample: return _showSample(st, samples);
    case slideshow::Plan::ShowPicture: return _showPicture(st, pictures);
    }
}

void DisplayImageLoop::_pause(const std::stop_token& st)
{
    phase_.moveTo(Phase::Paused);
    logger_->debug("Slideshow inactive, waiting");
    nextChangeAtMs_.store(0);
    std::unique_lock lk(mtx_);
    cv_.wait_for(lk, std::chrono::seconds(settings_.getEffectiveInterval()),
                 [&st, this] { return st.stop_requested() || settings_.isSlideshowActive(); });
}

void DisplayImageLoop::_waitForContent(const std::stop_token& st)
{
    phase_.moveTo(Phase::Empty);
    logger_->debug("No images available, waiting");
    _armNextChange();
    std::unique_lock lk(mtx_);
    cv_.wait_for(lk, std::chrono::seconds(settings_.getEffectiveInterval()),
                 [&st, this] { return st.stop_requested() || _endsHold(Content::None); });
    // Taken only now, so a request that came in before the wait still cuts it short.
    requests_.skip = false;
    requests_.newPicture = false;
}

void DisplayImageLoop::_showSample(const std::stop_token& st, const std::vector<std::filesystem::path>& samples)
{
    if (const int wait = display_.secondsUntilRefreshReady(); wait > 0)
    {
        phase_.moveTo(Phase::Yielding);
        std::unique_lock lk(mtx_);
        requests_.skip = false;
        requests_.newPicture = false;
        cv_.wait_for(lk, std::chrono::seconds(wait),
                     [&st, this] { return st.stop_requested() || _endsHold(Content::Sample); });
        return;
    }

    if (defaultImageIdx_ >= samples.size())
        defaultImageIdx_ = 0;
    const Frame frame{Content::Sample, samples[defaultImageIdx_], ""};

    phase_.moveTo(Phase::Painting);
    if (_paint(frame, st))
    {
        ready_.store(true);
        logger_->info("Displaying default image ({})", frame.path.string());
    }
    else if (st.stop_requested())
        return;
    else
        logger_->error("Failed to display default image: {}", frame.path.string());

    ++defaultImageIdx_;

    _armNextChange();
    {
        std::lock_guard lk(mtx_);
        requests_.skip = false;
        requests_.newPicture = false;
        phase_.moveTo(Phase::Holding);
    }
    _hold(st, frame);
}

void DisplayImageLoop::_showPicture(const std::stop_token& st, const std::vector<Image>& pictures)
{
    if (std::exchange(defaultImageIdx_, 0) != 0)
        logger_->info("User images available, switching from defaults");

    const int64_t nextId = _takeNext(pictures);
    const auto& image = *std::ranges::find(pictures, nextId, &Image::id);
    const Frame frame{Content::Picture, image.imagePath, image.sender};

    phase_.moveTo(Phase::Painting);
    if (_paint(frame, st))
    {
        ready_.store(true);
        logger_->info("Displaying image {} ({})", nextId, frame.path.string());
    }
    else if (st.stop_requested())
        return;
    else if (lastPaintUnreadable_)
    {
        // Moving on at once: holding it would leave the last picture up for a whole interval.
        logger_->warn("Image {} cannot be read, leaving it out until the next restart", nextId);
        unreadable_.insert(nextId);
        return;
    }
    else
        logger_->error("Failed to display image {}", nextId);

    currentImageId_ = nextId;

    _armNextChange();
    {
        std::lock_guard lk(mtx_);
        requests_.skip = false;
        phase_.moveTo(Phase::Holding);
    }
    _hold(st, frame);
}

void DisplayImageLoop::_recover(const std::stop_token& st)
{
    phase_.moveTo(Phase::Recovering);
    std::unique_lock lk(mtx_);
    cv_.wait_for(lk, std::chrono::seconds(5), [&st] { return st.stop_requested(); });
}

bool DisplayImageLoop::_paint(const Frame& frame, const std::stop_token& st)
{
    const bool sample = frame.content == Content::Sample;
    auto buffer = PhotoRenderer::render(frame.path);
    lastPaintUnreadable_ = buffer.empty();
    if (lastPaintUnreadable_)
        return false;
    const bool nightHold = settings_.nightState() == NightState::Hold;
    if (!buffer.empty() && nightHold)
    {
        const auto text = panel_badge::nightText(settings_.getNightEndHour());
        logger_->info("Holding for the night, drawing badge \"{}\"", text);
        panel_badge::draw(buffer, cfg_.display.boldFontPath, text, panel_badge::Corner::BottomRight);
    }
    if (!buffer.empty() && sample)
    {
        std::vector<panel_text::Rect> avoid;
        if (nightHold)
            avoid.push_back(panel_badge::area(panel_badge::Corner::BottomRight));
        if (lastNote_)
            avoid.push_back(*lastNote_);
        if (const auto note = panel_badge::drawAnywhere(buffer, cfg_.display.boldFontPath,
                                                        std::string(panel_badge::NO_IMAGES_TEXT), noteRng_, avoid))
            lastNote_ = note;
    }
    if (!buffer.empty() && !frame.sender.empty() && settings_.isShowSender())
        panel_badge::draw(buffer, cfg_.display.boldFontPath, frame.sender, panel_badge::Corner::TopRight);
    return display_.displayBuffer(buffer, frame.path.string(), st);
}

void DisplayImageLoop::_hold(const std::stop_token& st, const Frame& frame)
{
    using Clock = std::chrono::steady_clock;
    using std::chrono::milliseconds;
    const Clock::time_point due{milliseconds(nextChangeAtMs_.load())};
    const auto endsHold = [this, &frame] { return _endsHold(frame.content); };
    std::unique_lock lk(mtx_);
    while (true)
    {
        cv_.wait_until(lk, st, due, [&] { return endsHold() || requests_.panelWiped; });
        if (!std::exchange(requests_.panelWiped, false) || st.stop_requested() || endsHold())
            return;

        const auto refreshAllowedIn = std::chrono::seconds(display_.secondsUntilRefreshReady());
        const auto nextDueIn = std::chrono::duration_cast<milliseconds>(due - Clock::now());
        const auto response = slideshow::onPanelWiped(display_.isCleared(), milliseconds(refreshAllowedIn).count(),
                                                      nextDueIn.count());
        if (response == slideshow::WipeResponse::LeaveToNext)
            logger_->info("Panel cleared, next image due before a repaint is allowed");
        if (response != slideshow::WipeResponse::Repaint)
            continue;
        if (cv_.wait_until(lk, st, Clock::now() + refreshAllowedIn, endsHold) || st.stop_requested())
            return;

        phase_.moveTo(Phase::Painting);
        lk.unlock();
        if (_paint(frame, st))
            logger_->info("Repainted {} after the daily clear", frame.path.string());
        else if (!st.stop_requested())
            logger_->error("Failed to repaint {} after the daily clear", frame.path.string());
        lk.lock();
        phase_.moveTo(Phase::Holding);
    }
}

bool DisplayImageLoop::_endsHold(const Content held) const
{
    return slideshow::endsHold(held, requests_, settings_.isSlideshowActive(),
                               settings_.nightState() != NightState::Day);
}

void DisplayImageLoop::_onScheduledClear()
{
    std::lock_guard lk(mtx_);
    requests_.panelWiped = true;
    cv_.notify_one();
}

void DisplayImageLoop::_onImageNew()
{
    std::lock_guard lk(mtx_);
    requests_.newPicture = true;
    cv_.notify_one();
}

int64_t DisplayImageLoop::_pickNext(const std::vector<Image>& images)
{
    if (const auto mode = settings_.getPlaybackMode(); mode != PlaybackMode::Shuffle)
        return playback::nextAfter(playback::arrange(images, mode, settings_.getCustomOrder()),
                                   currentImageId_.load());

    std::vector<int64_t> ids;
    ids.reserve(images.size());
    for (const auto& img : images)
        ids.push_back(img.id);
    std::lock_guard lk(shuffleMtx_);
    shuffle_.sync(ids);
    return shuffle_.next();
}

std::vector<int64_t> DisplayImageLoop::shuffleOrder() const
{
    std::lock_guard lk(shuffleMtx_);
    return shuffle_.order();
}

void DisplayImageLoop::_onSkipImage()
{
    std::lock_guard lk(mtx_);
    logger_->info("Skip image requested via IPC");
    requests_.skip = true;
    cv_.notify_one();
}

void DisplayImageLoop::_onShowImage(const ShowImageEvent& evt)
{
    std::lock_guard lk(mtx_);
    logger_->info("Show image {} requested via IPC", evt.id);
    requests_.showId = evt.id;
    requests_.skip = true;
    cv_.notify_one();
}

int64_t DisplayImageLoop::_takeNext(const std::vector<Image>& images)
{
    int64_t requested;
    {
        std::lock_guard lk(mtx_);
        requested = std::exchange(requests_.showId, 0);
    }
    if (requested != 0 && std::ranges::find(images, requested, &Image::id) != images.end())
        return requested;
    return _pickNext(images);
}

void DisplayImageLoop::_onUpdateInterval(const UpdateDisplayIntervalEvent& evt) const
{
    logger_->info("Updating display interval to {}s", evt.intervalSecs);
    settings_.setDisplayInterval(evt.intervalSecs);
}

void DisplayImageLoop::_onUpdateNightMode(const UpdateNightModeEvent& evt) const
{
    settings_.setNightMode(evt.enabled, evt.startHour, evt.endHour, evt.intervalSecs, evt.style);
}

void DisplayImageLoop::_onUpdatePlaybackOrder(const UpdatePlaybackOrderEvent& evt) const
{
    settings_.setPlaybackOrder(evt.mode, evt.customOrder);
}

void DisplayImageLoop::_onUpdateCustomization(const UpdateCustomizationEvent& evt) const
{
    settings_.setShowSender(evt.showSender);
}

void DisplayImageLoop::_onSetSlideshowActive(const SetSlideshowActiveEvent& evt)
{
    std::lock_guard lk(mtx_);
    logger_->info("Slideshow active set to {}", evt.active);
    settings_.setSlideshowActive(evt.active);
    cv_.notify_one();
}

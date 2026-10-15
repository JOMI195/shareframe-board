#include "task/DisplayTaskManager.hpp"
#include "display/PhotoRenderer.hpp"
#include "settings/RuntimeSettings.hpp"

namespace
{
const char* name(const DisplayState state)
{
    switch (state)
    {
    case DisplayState::Boot: return "Boot";
    case DisplayState::Slideshow: return "Slideshow";
    case DisplayState::NightBlank: return "NightBlank";
    case DisplayState::ApSetup: return "ApSetup";
    }
    return "?";
}
} // namespace

DisplayTaskManager::DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                                       NightBlankScreen& nightBlank, const RuntimeSettings& settings)
    : Task(bus, "DisplayTaskManager"), cfg_(cfg), display_(display), slideshow_(slideshow),
      apSetup_(apSetup), nightBlank_(nightBlank), settings_(settings)
{
}

void DisplayTaskManager::subscribe()
{
    bus_.subscribe<Topic::AP_SETUP_STATE>([this](const ApSetupStateEvent& evt)
    {
        {
            std::lock_guard lk(mtx_);
            latest_ = evt;
            changed_ = true;
        }
        cv_.notify_one();
    });
    // Runs after the slideshow's handler stored the new night mode, as long as
    // the slideshow subscribes first.
    bus_.subscribe<Topic::UPDATE_NIGHT_MODE>([this](const UpdateNightModeEvent&)
    {
        {
            std::lock_guard lk(mtx_);
            changed_ = true;
        }
        cv_.notify_one();
    });
}

void DisplayTaskManager::_run(const std::stop_token st)
{
    if (!booted_)
    {
        _boot(st);
        booted_ = true;
    }

    while (!st.stop_requested())
    {
        ApSetupStateEvent apSetup;
        {
            std::unique_lock lk(mtx_);
            if (const int untilNightChange = settings_.secsUntilNightBlankChange(); untilNightChange >= 0)
                cv_.wait_for(lk, st, std::chrono::seconds(untilNightChange), [this] { return changed_; });
            else
                cv_.wait(lk, st, [this] { return changed_; });
            if (st.stop_requested())
                break;
            apSetup = latest_;
            changed_ = false;
        }
        _apply(apSetup);
    }
    _stopActive();
}

void DisplayTaskManager::_boot(const std::stop_token& st)
{
    logger_->info("State Boot");
    display_.init();
    const auto loadingImage = std::filesystem::path(cfg_.display.loadingImagePath) / "logo-frame-loading-shareframe.jpg";
    if (!display_.displayBuffer(PhotoRenderer::render(loadingImage), loadingImage.string(), st,
                                RefreshPolicy::Immediate) && !st.stop_requested())
        logger_->error("Failed to display loading image");
}

void DisplayTaskManager::_apply(const ApSetupStateEvent& apSetup)
{
    const auto current = state_.load();
    const auto target = apSetup.required ? DisplayState::ApSetup
        : settings_.isNightBlankNow() ? DisplayState::NightBlank
        : DisplayState::Slideshow;
    if (target == current)
    {
        if (target == DisplayState::ApSetup)
            apSetup_.setCredentials(apSetup.ssid, apSetup.password);
        return;
    }

    logger_->info("State {} -> {}", name(current), name(target));
    _stopActive();
    if (current == DisplayState::ApSetup && !settings_.isSlideshowActive())
        display_.clear();

    if (target == DisplayState::ApSetup)
    {
        apSetup_.setCredentials(apSetup.ssid, apSetup.password);
        apSetup_.start();
    }
    else if (target == DisplayState::NightBlank)
        nightBlank_.start();
    else
        slideshow_.start();
    state_.store(target);
}

void DisplayTaskManager::_stopActive()
{
    switch (state_.load())
    {
    case DisplayState::Slideshow: slideshow_.stop(); break;
    case DisplayState::NightBlank: nightBlank_.stop(); break;
    case DisplayState::ApSetup: apSetup_.stop(); break;
    case DisplayState::Boot: break;
    }
}

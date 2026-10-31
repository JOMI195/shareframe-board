#include "task/DisplayTaskManager.hpp"
#include "display/LogoScreenRenderer.hpp"
#include "settings/RuntimeSettings.hpp"
#include "Version.hpp"
#include <utility>

namespace
{
int64_t steadySecs()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

const char* name(const DisplayState state)
{
    switch (state)
    {
    case DisplayState::Boot: return "Boot";
    case DisplayState::Slideshow: return "Slideshow";
    case DisplayState::NightBlank: return "NightBlank";
    case DisplayState::Registration: return "Registration";
    case DisplayState::ApSetup: return "ApSetup";
    case DisplayState::Status: return "Status";
    }
    return "?";
}
} // namespace

DisplayTaskManager::DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                                       RegistrationScreen& registration, NightBlankScreen& nightBlank,
                                       StatusScreen& status, const RuntimeSettings& settings)
    : Task(bus, "DisplayTaskManager"), cfg_(cfg), display_(display), slideshow_(slideshow),
      apSetup_(apSetup), registration_(registration), nightBlank_(nightBlank), status_(status),
      settings_(settings)
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
    bus_.subscribe<Topic::STATUS_SCREEN>([this](const StatusScreenEvent& evt)
    {
        {
            std::lock_guard lk(mtx_);
            latestStatus_ = evt;
            statusSince_ = std::chrono::steady_clock::now();
            changed_ = true;
        }
        cv_.notify_one();
    });
    bus_.subscribe<Topic::FRAME_REGISTRATION>([this](const FrameRegistrationEvent& evt)
    {
        {
            std::lock_guard lk(mtx_);
            registrationWindow_.report(evt.registered, steadySecs(), cfg_.display.registrationScreenWindowSecs);
            changed_ = true;
        }
        cv_.notify_one();
    });
    bus_.subscribe<Topic::SCHEDULED_CLEAR>([this](const ScheduledClearEvent&)
    {
        {
            std::lock_guard lk(mtx_);
            restore_ = true;
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
        StatusScreenEvent status;
        bool registration;
        bool restore;
        {
            std::unique_lock lk(mtx_);
            const auto statusTimeout = std::chrono::seconds(cfg_.display.statusScreenTimeoutSecs);
            std::optional<std::chrono::steady_clock::time_point> wakeAt;
            if (const int untilNightChange = settings_.secsUntilNightBlankChange(); untilNightChange >= 0)
                wakeAt = std::chrono::steady_clock::now() + std::chrono::seconds(untilNightChange);
            if (latestStatus_.kind)
                wakeAt = std::min(wakeAt.value_or(std::chrono::steady_clock::time_point::max()),
                                  statusSince_ + statusTimeout);
            if (const auto ends = registrationWindow_.endsAt(steadySecs()))
                wakeAt = std::min(wakeAt.value_or(std::chrono::steady_clock::time_point::max()),
                                  std::chrono::steady_clock::time_point(std::chrono::seconds(*ends)));
            if (wakeAt)
                cv_.wait_until(lk, st, *wakeAt, [this] { return changed_; });
            else
                cv_.wait(lk, st, [this] { return changed_; });
            if (st.stop_requested())
                break;
            if (latestStatus_.kind && std::chrono::steady_clock::now() >= statusSince_ + statusTimeout)
            {
                logger_->warn("Status screen {} still up after {}s, hiding it", toString(*latestStatus_.kind),
                              cfg_.display.statusScreenTimeoutSecs);
                latestStatus_ = {};
            }
            apSetup = latest_;
            status = latestStatus_;
            registration = registrationWindow_.required(steadySecs());
            restore = std::exchange(restore_, false);
            changed_ = false;
        }
        _apply(apSetup, status, registration, restore);
    }
    _stopActive();
}

void DisplayTaskManager::_boot(const std::stop_token& st)
{
    logger_->info("State Boot");
    display_.init();
    const auto loadingImage = std::filesystem::path(cfg_.display.loadingImagePath) / LogoScreenRenderer::IMAGE;
    const auto buffer = LogoScreenRenderer::render(loadingImage, "Wird geladen …",
                                                   LogoScreenRenderer::versionLabel(cfg_.version, shareframe::kAppSha),
                                                   cfg_.display.boldFontPath, cfg_.display.setupFontPath);
    if (!display_.displayBuffer(buffer, loadingImage.string(), st, RefreshPolicy::Immediate) && !st.stop_requested())
        logger_->error("Failed to display loading image");
}

void DisplayTaskManager::_apply(const ApSetupStateEvent& apSetup, const StatusScreenEvent& status,
                                const bool registration, const bool restore)
{
    const auto current = state_.load();
    const auto target = status.kind ? DisplayState::Status
        : apSetup.required ? DisplayState::ApSetup
        : registration ? DisplayState::Registration
        : settings_.isNightBlankNow() ? DisplayState::NightBlank
        : DisplayState::Slideshow;
    const bool stale = (target == DisplayState::Status && (restore || status != shownStatus_))
        || (target == DisplayState::NightBlank && (restore || settings_.getNightEndHour() != shownNightEnd_));
    if (target == current && !stale)
    {
        if (target == DisplayState::ApSetup)
            apSetup_.setCredentials(apSetup.ssid, apSetup.password);
        return;
    }

    logger_->info("State {} -> {}", name(current), name(target));
    _stopActive();
    const bool leavesScreen = current != DisplayState::Boot && current != DisplayState::Slideshow
        && target == DisplayState::Slideshow;
    if (leavesScreen && !settings_.isSlideshowActive())
        display_.clear();

    if (target == DisplayState::Status)
    {
        status_.set(*status.kind, status.version);
        status_.start();
        shownStatus_ = status;
    }
    else if (target == DisplayState::ApSetup)
    {
        apSetup_.setCredentials(apSetup.ssid, apSetup.password);
        apSetup_.start();
    }
    else if (target == DisplayState::Registration)
        registration_.start();
    else if (target == DisplayState::NightBlank)
    {
        shownNightEnd_ = settings_.getNightEndHour();
        nightBlank_.set(shownNightEnd_);
        nightBlank_.start();
    }
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
    case DisplayState::Registration: registration_.stop(); break;
    case DisplayState::Status: status_.stop(); break;
    case DisplayState::Boot: break;
    }
}

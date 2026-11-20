#include "task/DisplayTaskManager.hpp"
#include "display/LogoScreenRenderer.hpp"
#include "settings/RuntimeSettings.hpp"
#include "Version.hpp"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <utility>

namespace
{
using Clock = std::chrono::steady_clock;

int64_t steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
}
} // namespace

DisplayTaskManager::DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                                       RegistrationScreen& registration, NightBlankScreen& nightBlank,
                                       StatusScreen& status, const RuntimeSettings& settings)
    : Task(bus, "DisplayTaskManager"), cfg_(cfg), display_(display), slideshow_(slideshow),
      apSetup_(apSetup), registration_(registration), nightBlank_(nightBlank), status_(status),
      settings_(settings), screen_(Screen::Boot, logger_)
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
            statusWindow_.request(evt, steadyMs());
            changed_ = true;
        }
        cv_.notify_one();
    });
    bus_.subscribe<Topic::FRAME_REGISTRATION>([this](const FrameRegistrationEvent& evt)
    {
        if (!evt.claimCode.empty())
            registration_.setClaimCode(evt.claimCode);
        std::error_code ec;
        const bool ownedBefore = std::filesystem::exists(cfg_.display.registeredOncePath, ec);
        if (evt.registered && !ownedBefore)
            _rememberOwned();
        {
            std::lock_guard lk(mtx_);
            registrationWindow_.report(evt.registered, steadyMs(),
                                       int64_t{cfg_.display.registrationScreenWindowSecs} * 1000, ownedBefore);
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
    {
        // A run that starts over after a failed switch has to look again.
        std::lock_guard lk(mtx_);
        changed_ = true;
    }

    while (const auto wishes = _awaitChange(st))
        _carryOut(_decide(*wishes), *wishes);
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

std::optional<DisplayTaskManager::Wishes> DisplayTaskManager::_awaitChange(const std::stop_token& st)
{
    std::unique_lock lk(mtx_);
    if (const auto deadline = _nextDeadline())
        cv_.wait_until(lk, st, *deadline, [this] { return changed_; });
    else
        cv_.wait(lk, st, [this] { return changed_; });
    if (st.stop_requested())
        return std::nullopt;

    const int timeoutSecs = cfg_.display.statusScreenTimeoutSecs;
    if (const auto forgotten = statusWindow_.expire(steadyMs(), int64_t{timeoutSecs} * 1000))
        logger_->warn("Status screen {} still up after {}s, hiding it", toString(*forgotten), timeoutSecs);
    changed_ = false;
    return Wishes{latest_, statusWindow_.requested(), registrationWindow_.required(steadyMs()),
                  std::exchange(restore_, false)};
}

std::optional<Clock::time_point> DisplayTaskManager::_nextDeadline() const
{
    std::optional<Clock::time_point> earliest;
    const auto consider = [&earliest](const Clock::time_point at)
    {
        earliest = earliest ? std::min(*earliest, at) : at;
    };
    if (const int untilNightChange = settings_.secsUntilNightBlankChange(); untilNightChange >= 0)
        consider(Clock::now() + std::chrono::seconds(untilNightChange));
    if (const auto ends = statusWindow_.endsAt(int64_t{cfg_.display.statusScreenTimeoutSecs} * 1000))
        consider(Clock::time_point(std::chrono::milliseconds(*ends)));
    if (const auto ends = registrationWindow_.endsAt(steadyMs()))
        consider(Clock::time_point(std::chrono::milliseconds(*ends)));
    return earliest;
}

ScreenDecision DisplayTaskManager::_decide(const Wishes& wishes) const
{
    const ScreenInputs inputs{
        .statusRequested = wishes.status.kind.has_value(),
        .apSetupRequired = wishes.apSetup.required,
        .registrationRequired = wishes.registration,
        .nightBlank = settings_.nightState() == NightState::Pause,
    };
    // These two paint once, so only a restart brings new or wiped content back.
    bool stale = false;
    if (const Screen target = select(inputs); target == Screen::Status)
        stale = wishes.panelWiped || wishes.status != shownStatus_;
    else if (target == Screen::NightBlank)
        stale = wishes.panelWiped || settings_.getNightEndHour() != shownNightEnd_;
    return decide(screen_.state(), inputs, stale, settings_.isSlideshowActive());
}

void DisplayTaskManager::_carryOut(const ScreenDecision& decision, const Wishes& wishes)
{
    if (decision.action == ScreenAction::Keep)
    {
        if (decision.target == Screen::ApSetup)
            apSetup_.setCredentials(wishes.apSetup.ssid, wishes.apSetup.password, wishes.apSetup.code);
        return;
    }

    _stopActive();
    screen_.moveTo(Screen::Switching, name(decision.target));
    if (decision.clearFirst)
        display_.clear();
    _prepare(decision.target, wishes);
    _task(decision.target)->start();
    screen_.moveTo(decision.target);
}

void DisplayTaskManager::_prepare(const Screen target, const Wishes& wishes)
{
    if (target == Screen::Status)
    {
        status_.set(*wishes.status.kind, wishes.status.version, wishes.status.code);
        shownStatus_ = wishes.status;
    }
    else if (target == Screen::ApSetup)
        apSetup_.setCredentials(wishes.apSetup.ssid, wishes.apSetup.password, wishes.apSetup.code);
    else if (target == Screen::NightBlank)
    {
        shownNightEnd_ = settings_.getNightEndHour();
        nightBlank_.set(shownNightEnd_);
    }
}

Task* DisplayTaskManager::_task(const Screen screen) const
{
    switch (screen)
    {
    case Screen::Slideshow: return &slideshow_;
    case Screen::NightBlank: return &nightBlank_;
    case Screen::Registration: return &registration_;
    case Screen::ApSetup: return &apSetup_;
    case Screen::Status: return &status_;
    case Screen::Boot:
    case Screen::Switching: break;
    }
    return nullptr;
}

void DisplayTaskManager::_stopActive()
{
    if (Task* active = _task(screen_.state()))
        active->stop();
}

void DisplayTaskManager::_rememberOwned() const
{
    const std::filesystem::path flag(cfg_.display.registeredOncePath);
    std::error_code ec;
    std::filesystem::create_directories(flag.parent_path(), ec);
    const auto tmp = flag.string() + ".tmp";
    if (std::ofstream f(tmp); !(f << std::time(nullptr) << '\n'))
    {
        logger_->warn("Cannot write {}, a later registration screen will not time out", tmp);
        return;
    }
    std::filesystem::rename(tmp, flag, ec);
    if (ec)
        logger_->warn("Cannot write {}: {}", flag.string(), ec.message());
}

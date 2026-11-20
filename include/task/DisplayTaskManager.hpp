#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "display/RegistrationWindow.hpp"
#include "display/ScreenSelection.hpp"
#include "display/StatusWindow.hpp"
#include "events/Messages.hpp"
#include "fsm/Machine.hpp"
#include "task/ApNetworkSetupScreen.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/NightBlankScreen.hpp"
#include "task/RegistrationScreen.hpp"
#include "task/StatusScreen.hpp"
#include "task/Task.hpp"
#include <chrono>
#include <optional>

class RuntimeSettings;

/// Decides what the panel shows: boots it with the loading screen, then runs exactly
/// one content task, picked by the screen selection rules. Switching stops the running
/// task first, which cancels a paint still waiting for the refresh interval. Runs on
/// its own thread because stopping a task blocks.
class DisplayTaskManager : public Task
{
public:
    DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                       RegistrationScreen& registration, NightBlankScreen& nightBlank, StatusScreen& status,
                       const RuntimeSettings& settings);

    /// Call once before the bus starts.
    void subscribe();

    [[nodiscard]] Screen state() const { return screen_.state(); }

protected:
    void _run(std::stop_token st) override;

private:
    void _rememberOwned() const;
    /// What the other services ask for, as of one evaluation.
    struct Wishes
    {
        ApSetupStateEvent apSetup;
        StatusScreenEvent status;
        bool registration = false;
        bool panelWiped = false;
    };

    void _boot(const std::stop_token& st);
    /// Blocks until something changed or a deadline passed; nothing on a stop request.
    [[nodiscard]] std::optional<Wishes> _awaitChange(const std::stop_token& st);
    /// Call under lock.
    [[nodiscard]] std::optional<std::chrono::steady_clock::time_point> _nextDeadline() const;
    [[nodiscard]] ScreenDecision _decide(const Wishes& wishes) const;
    void _carryOut(const ScreenDecision& decision, const Wishes& wishes);
    void _prepare(Screen target, const Wishes& wishes);
    [[nodiscard]] Task* _task(Screen screen) const;
    void _stopActive();

    const AppConfig& cfg_;
    DisplayManager& display_;
    DisplayImageLoop& slideshow_;
    ApNetworkSetupScreen& apSetup_;
    RegistrationScreen& registration_;
    NightBlankScreen& nightBlank_;
    StatusScreen& status_;
    const RuntimeSettings& settings_;
    ApSetupStateEvent latest_;
    RegistrationWindow registrationWindow_;
    StatusWindow statusWindow_;
    StatusScreenEvent shownStatus_;
    int shownNightEnd_ = -1;
    bool changed_ = true;
    bool restore_ = false;
    bool booted_ = false;
    fsm::Machine<Screen> screen_;
};

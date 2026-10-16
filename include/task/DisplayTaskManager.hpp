#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "events/Messages.hpp"
#include "task/ApNetworkSetupScreen.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/NightBlankScreen.hpp"
#include "task/StatusScreen.hpp"
#include "task/Task.hpp"
#include <atomic>

class RuntimeSettings;

enum class DisplayState
{
    Boot,
    Slideshow,
    NightBlank,
    ApSetup,
    Status
};

/// Decides what the panel shows: boots it with the loading screen, then runs exactly
/// one content task. A status screen outranks the Wi-Fi setup screen, which outranks
/// a blank night, which outranks the slideshow. Switching stops
/// the running task first, which cancels a paint still waiting for the refresh
/// interval. Runs on its own thread because stopping a task blocks.
class DisplayTaskManager : public Task
{
public:
    DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                       NightBlankScreen& nightBlank, StatusScreen& status, const RuntimeSettings& settings);

    /// Call once before the bus starts.
    void subscribe();

    [[nodiscard]] DisplayState state() const { return state_.load(); }

protected:
    void _run(std::stop_token st) override;

private:
    void _boot(const std::stop_token& st);
    void _apply(const ApSetupStateEvent& apSetup, const StatusScreenEvent& status);
    void _stopActive();

    const AppConfig& cfg_;
    DisplayManager& display_;
    DisplayImageLoop& slideshow_;
    ApNetworkSetupScreen& apSetup_;
    NightBlankScreen& nightBlank_;
    StatusScreen& status_;
    const RuntimeSettings& settings_;
    ApSetupStateEvent latest_;
    StatusScreenEvent latestStatus_;
    std::chrono::steady_clock::time_point statusSince_;
    StatusScreenEvent shownStatus_;
    int shownNightEnd_ = -1;
    bool changed_ = true;
    bool booted_ = false;
    std::atomic<DisplayState> state_{DisplayState::Boot};
};

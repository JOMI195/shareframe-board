#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "events/Messages.hpp"
#include "task/ApNetworkSetupScreen.hpp"
#include "task/DisplayImageLoop.hpp"
#include "task/NightBlankScreen.hpp"
#include "task/Task.hpp"
#include <atomic>

class RuntimeSettings;

enum class DisplayState
{
    Boot,
    Slideshow,
    NightBlank,
    ApSetup
};

/// Decides what the panel shows: boots it with the loading image, then runs exactly
/// one content task. The Wi-Fi setup screen outranks a blank night, which outranks
/// the slideshow. Switching stops
/// the running task first, which cancels a paint still waiting for the refresh
/// interval. Runs on its own thread because stopping a task blocks.
class DisplayTaskManager : public Task
{
public:
    DisplayTaskManager(EventBus& bus, const AppConfig& cfg, DisplayManager& display,
                       DisplayImageLoop& slideshow, ApNetworkSetupScreen& apSetup,
                       NightBlankScreen& nightBlank, const RuntimeSettings& settings);

    /// Call once before the bus starts.
    void subscribe();

    [[nodiscard]] DisplayState state() const { return state_.load(); }

protected:
    void _run(std::stop_token st) override;

private:
    void _boot(const std::stop_token& st);
    void _apply(const ApSetupStateEvent& apSetup);
    void _stopActive();

    const AppConfig& cfg_;
    DisplayManager& display_;
    DisplayImageLoop& slideshow_;
    ApNetworkSetupScreen& apSetup_;
    NightBlankScreen& nightBlank_;
    const RuntimeSettings& settings_;
    ApSetupStateEvent latest_;
    bool changed_ = true;
    bool booted_ = false;
    std::atomic<DisplayState> state_{DisplayState::Boot};
};

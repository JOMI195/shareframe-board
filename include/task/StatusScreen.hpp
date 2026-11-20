#pragma once
#include "config/AppConfig.hpp"
#include "display/DisplayManager.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "task/Task.hpp"
#include <string>
#include <vector>

/// Shows the logo with what the frame is busy with (update, reboot, reset). Started
/// and stopped by DisplayTaskManager; paints once, within the refresh interval.
class StatusScreen : public Task
{
public:
    StatusScreen(EventBus& bus, const AppConfig& cfg, DisplayManager& display);

    /// Call while stopped. `code` is the reset code a ResetCode screen shows.
    void set(StatusScreenKind kind, const std::string& version, const std::string& code = "");

    /// After stop(): the shutdown refresh paints the screen if it never made it
    /// to the panel, so it stays up through the reboot.
    void finishForShutdown();

protected:
    void _run(std::stop_token st) override;

private:
    const AppConfig& cfg_;
    DisplayManager& display_;
    std::vector<uint8_t> buffer_;
    std::vector<std::string> text_;
    std::string source_;
    bool shown_ = false;
};

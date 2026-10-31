#pragma once
#include <cstdint>
#include <string_view>

/// Decides from the Wi-Fi daemon's mode whether the panel shows the AP setup screen.
/// It shows only within the boot window and before the first connection, so outages keep
/// the photos up until the next reboot. Station retries (`connecting`) keep the verdict,
/// so they don't flap the panel.
class ApSetupDebounce
{
public:
    explicit ApSetupDebounce(const int64_t bootWindowSecs) : bootWindowSecs_(bootWindowSecs) {}

    bool update(const std::string_view mode, const int64_t uptimeSecs)
    {
        const bool allowed = !seenConnected_ && uptimeSecs < bootWindowSecs_;
        if (mode == "connected")
        {
            seenConnected_ = true;
            required_ = false;
        }
        else if (mode == "ap")
            required_ = allowed;
        else
            required_ = required_ && allowed;
        return required_;
    }

private:
    int64_t bootWindowSecs_;
    bool seenConnected_ = false;
    bool required_ = false;
};

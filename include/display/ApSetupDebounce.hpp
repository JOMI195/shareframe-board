#pragma once
#include <cstdint>
#include <string_view>

/// Decides from the Wi-Fi daemon's mode whether the panel shows the AP setup screen.
/// A frame without saved networks always shows it in AP mode. With saved networks it
/// shows only within the boot window and before the first connection, so outages keep
/// the photos up. Station retries (`connecting`) keep the verdict, so they don't flap the panel.
class ApSetupDebounce
{
public:
    explicit ApSetupDebounce(const int64_t bootWindowSecs) : bootWindowSecs_(bootWindowSecs) {}

    bool update(const std::string_view mode, const bool knownNetworks, const int64_t uptimeSecs)
    {
        const bool allowed = !knownNetworks || (!seenConnected_ && uptimeSecs < bootWindowSecs_);
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

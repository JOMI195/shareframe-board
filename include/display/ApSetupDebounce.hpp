#pragma once
#include <string>
#include <string_view>

/// Decides from the Wi-Fi daemon's mode whether the panel shows the AP setup screen.
/// Once a network was joined, setup waits until a station retry has failed too, so a
/// short outage (router reboot) never costs a panel refresh. Station retries
/// (`connecting`) keep the verdict, so they don't flap the panel.
class ApSetupDebounce
{
public:
    bool update(const std::string_view mode)
    {
        if (mode == "connected")
        {
            seenConnected_ = true;
            apEntries_ = 0;
            required_ = false;
        }
        else if (mode == "ap")
        {
            if (lastMode_ != "ap")
                ++apEntries_;
            if (!seenConnected_ || apEntries_ >= 2)
                required_ = true;
        }
        lastMode_ = mode;
        return required_;
    }

private:
    std::string lastMode_;
    bool seenConnected_ = false;
    int apEntries_ = 0;
    bool required_ = false;
};

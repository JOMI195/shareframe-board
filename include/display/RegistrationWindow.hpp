#pragma once
#include <cstdint>
#include <optional>

/// Decides from the websocket's reports whether the panel asks to register the frame.
/// A frame owned before shows the screen from the first refusal for `windowMs`, then not until
/// the next reboot; a frame never owned shows it until the server takes it.
class RegistrationWindow
{
public:
    void report(const bool registered, const int64_t nowMs, const int64_t windowMs, const bool ownedBefore = true)
    {
        if (registered)
            registered_ = true;
        else if (!until_ && !untilClaimed_)
        {
            if (ownedBefore)
                until_ = nowMs + windowMs;
            else
                untilClaimed_ = true;
        }
    }

    [[nodiscard]] bool required(const int64_t nowMs) const
    {
        return !registered_ && (untilClaimed_ || (until_ && nowMs < *until_));
    }

    /// When a showing screen times out; nothing while it doesn't show or has no deadline.
    [[nodiscard]] std::optional<int64_t> endsAt(const int64_t nowMs) const
    {
        return required(nowMs) ? until_ : std::nullopt;
    }

private:
    std::optional<int64_t> until_;
    bool untilClaimed_ = false;
    bool registered_ = false;
};

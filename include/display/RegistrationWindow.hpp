#pragma once
#include <cstdint>
#include <optional>

/// Decides from the websocket's reports whether the panel asks to register the frame.
/// The screen shows from the first refusal for `windowMs`, or until the server takes
/// the frame; after that it stays off until the next reboot.
class RegistrationWindow
{
public:
    void report(const bool registered, const int64_t nowMs, const int64_t windowMs)
    {
        if (registered)
            registered_ = true;
        else if (!until_)
            until_ = nowMs + windowMs;
    }

    [[nodiscard]] bool required(const int64_t nowMs) const { return !registered_ && until_ && nowMs < *until_; }

    /// When a showing screen times out; nothing while it doesn't show.
    [[nodiscard]] std::optional<int64_t> endsAt(const int64_t nowMs) const
    {
        return required(nowMs) ? until_ : std::nullopt;
    }

private:
    std::optional<int64_t> until_;
    bool registered_ = false;
};

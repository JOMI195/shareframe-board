#pragma once
#include "events/Messages.hpp"
#include <cstdint>
#include <optional>
#include <utility>

/// Holds the status screen another service asked for until that service hides it. A
/// screen nobody hid in time is forgotten, so a failed reboot doesn't keep it up forever.
class StatusWindow
{
public:
    void request(StatusScreenEvent event, const int64_t nowMs)
    {
        event_ = std::move(event);
        since_ = nowMs;
    }

    /// Forgets a screen that outlived its timeout and returns its kind.
    std::optional<StatusScreenKind> expire(const int64_t nowMs, const int64_t timeoutMs)
    {
        if (!event_.kind || nowMs < since_ + timeoutMs)
            return std::nullopt;
        return std::exchange(event_, {}).kind;
    }

    [[nodiscard]] const StatusScreenEvent& requested() const { return event_; }

    /// When a requested screen times out; nothing while none is requested.
    [[nodiscard]] std::optional<int64_t> endsAt(const int64_t timeoutMs) const
    {
        return event_.kind ? std::optional(since_ + timeoutMs) : std::nullopt;
    }

private:
    StatusScreenEvent event_;
    int64_t since_ = 0;
};

#pragma once
#include <optional>
#include <string_view>

/// Where an A/B update stands, from the server check to the reboot into the new slot.
enum class UpdatePhase
{
    Idle,
    Checking,
    Downloading,
    Installing,
    AwaitingReboot,
    Failed
};

constexpr std::string_view toString(const UpdatePhase phase)
{
    switch (phase)
    {
    case UpdatePhase::Idle: return "idle";
    case UpdatePhase::Checking: return "checking";
    case UpdatePhase::Downloading: return "downloading";
    case UpdatePhase::Installing: return "installing";
    case UpdatePhase::AwaitingReboot: return "awaiting-reboot";
    case UpdatePhase::Failed: return "failed";
    }
    return "unknown";
}

constexpr std::string_view name(const UpdatePhase phase)
{
    return toString(phase);
}

constexpr std::optional<UpdatePhase> updatePhaseFromString(const std::string_view s)
{
    for (const auto phase : {UpdatePhase::Idle, UpdatePhase::Checking, UpdatePhase::Downloading,
                             UpdatePhase::Installing, UpdatePhase::AwaitingReboot, UpdatePhase::Failed})
    {
        if (toString(phase) == s)
            return phase;
    }
    return std::nullopt;
}

/// An update only moves forward, and every step of it may fail. A new one starts from
/// rest or after a failure.
constexpr bool canMove(const UpdatePhase from, const UpdatePhase to)
{
    switch (from)
    {
    case UpdatePhase::Idle:
    case UpdatePhase::Failed:
        return to == UpdatePhase::Checking;
    case UpdatePhase::Checking:
        return to == UpdatePhase::Downloading || to == UpdatePhase::Failed;
    case UpdatePhase::Downloading:
        return to == UpdatePhase::Installing || to == UpdatePhase::Failed;
    case UpdatePhase::Installing:
        return to == UpdatePhase::AwaitingReboot || to == UpdatePhase::Failed;
    case UpdatePhase::AwaitingReboot:
        return to == UpdatePhase::Failed;
    }
    return false;
}

/// While busy, no second update starts and the frame must not be rebooted from outside.
constexpr bool isBusy(const UpdatePhase phase)
{
    return phase != UpdatePhase::Idle && phase != UpdatePhase::Failed;
}

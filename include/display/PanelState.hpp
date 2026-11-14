#pragma once
#include <string_view>

enum class PanelPower
{
    Off,
    Asleep,
    Awake
};

constexpr std::string_view name(const PanelPower power)
{
    switch (power)
    {
    case PanelPower::Off: return "Off";
    case PanelPower::Asleep: return "Asleep";
    case PanelPower::Awake: return "Awake";
    }
    return "?";
}

/// The module only powers down from deep sleep, never while the panel is awake.
constexpr bool canMove(const PanelPower from, const PanelPower to)
{
    switch (from)
    {
    case PanelPower::Off: return to == PanelPower::Awake;
    case PanelPower::Asleep: return to == PanelPower::Awake || to == PanelPower::Off;
    case PanelPower::Awake: return to == PanelPower::Asleep;
    }
    return false;
}

/// What the last successful refresh left on the panel; unknown before the first one.
enum class PanelContent
{
    Unknown,
    Blank,
    Image
};

constexpr std::string_view toString(const PanelContent content)
{
    switch (content)
    {
    case PanelContent::Unknown: return "empty";
    case PanelContent::Blank: return "cleared";
    case PanelContent::Image: return "image";
    }
    return "empty";
}

enum class PanelHealth
{
    Ok,
    Degraded,
    Failed
};

/// The panel has no sensor, so health follows the streak of failed refreshes.
constexpr PanelHealth panelHealth(const int consecutiveFailures)
{
    if (consecutiveFailures == 0)
        return PanelHealth::Ok;
    return consecutiveFailures < 3 ? PanelHealth::Degraded : PanelHealth::Failed;
}

constexpr std::string_view toString(const PanelHealth health)
{
    switch (health)
    {
    case PanelHealth::Ok: return "ok";
    case PanelHealth::Degraded: return "degraded";
    case PanelHealth::Failed: return "failed";
    }
    return "failed";
}

#pragma once
#include <optional>
#include <string>

/// Logo screens other services ask the display to show while the frame reboots or installs.
enum class StatusScreenKind
{
    Update,
    Reboot,
    FactoryReset,
    ResetCode
};

inline std::string toString(const StatusScreenKind kind)
{
    switch (kind)
    {
    case StatusScreenKind::Update: return "update";
    case StatusScreenKind::Reboot: return "reboot";
    case StatusScreenKind::FactoryReset: return "factory_reset";
    case StatusScreenKind::ResetCode: return "reset_code";
    }
    return "unknown";
}

inline std::optional<StatusScreenKind> statusScreenKindFromString(const std::string& s)
{
    if (s == "update") return StatusScreenKind::Update;
    if (s == "reboot") return StatusScreenKind::Reboot;
    if (s == "factory_reset") return StatusScreenKind::FactoryReset;
    if (s == "reset_code") return StatusScreenKind::ResetCode;
    return std::nullopt;
}

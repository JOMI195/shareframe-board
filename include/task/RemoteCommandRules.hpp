#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace remote
{
/// What an admin may ask of a frame over the websocket.
enum class Command
{
    RequestDiagnostics,
    RestartService,
    Reboot,
    UpdateInstall,
    FactoryReset,
    Shutdown,
    EnterSafeMode,
    ExitSafeMode
};

constexpr std::optional<Command> commandFromString(const std::string_view s)
{
    if (s == "request_diagnostics") return Command::RequestDiagnostics;
    if (s == "restart_service") return Command::RestartService;
    if (s == "reboot") return Command::Reboot;
    if (s == "update_install") return Command::UpdateInstall;
    if (s == "factory_reset") return Command::FactoryReset;
    if (s == "shutdown") return Command::Shutdown;
    if (s == "enter_safe_mode") return Command::EnterSafeMode;
    if (s == "exit_safe_mode") return Command::ExitSafeMode;
    return std::nullopt;
}

/// Commands that take the frame down, so the board has to agree first.
constexpr bool isRebootClass(const Command command)
{
    return command == Command::Reboot || command == Command::UpdateInstall || command == Command::FactoryReset
        || command == Command::Shutdown;
}

constexpr bool isSafeMode(const Command command)
{
    return command == Command::EnterSafeMode || command == Command::ExitSafeMode;
}

constexpr bool isKnownService(const std::string_view service)
{
    constexpr std::array<std::string_view, 5> kServices = {"display", "websocket", "dashboard", "heartbeat", "update"};
    for (const auto known : kServices)
    {
        if (known == service)
            return true;
    }
    return false;
}

/// Commands that would cut an update short and so wait until none is under way.
constexpr bool needsIdleUpdate(const Command command, const std::string_view arg)
{
    return isRebootClass(command) || isSafeMode(command) || (command == Command::RestartService && arg == "update");
}

/// Why a command is refused before anything runs; nothing when it may go on.
constexpr std::optional<std::string_view> refusal(const std::optional<Command> command, const int64_t id,
                                                  const std::string_view arg, const int64_t nowSecs,
                                                  const int64_t expiresAtSecs)
{
    if (id <= 0 || !command)
        return "unknown command";
    if (*command == Command::RestartService ? !isKnownService(arg) : !arg.empty())
        return "invalid argument";
    if (nowSecs > expiresAtSecs)
        return "expired";
    return std::nullopt;
}
} // namespace remote

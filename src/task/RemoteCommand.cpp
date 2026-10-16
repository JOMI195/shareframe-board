#include "task/RemoteCommand.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "net/WsProtocol.hpp"
#include "util/Subprocess.hpp"
#include <algorithm>
#include <array>
#include <thread>

namespace
{
constexpr std::array kServices = {"display", "websocket", "dashboard", "heartbeat", "update"};

bool isKnownService(const std::string& s)
{
    return std::ranges::find(kServices, s) != kServices.end();
}

bool isRebootClass(const std::string& command)
{
    return command == "reboot" || command == "update_install" || command == "factory_reset";
}
}

RemoteCommand::RemoteCommand(EventBus& bus, IpcClient& displayIpc, IpcClient& updateIpc,
                             const DiagnosticsUploader& diagnostics, Timing timing)
    : Task(bus, "RemoteCommand"), displayIpc_(displayIpc), updateIpc_(updateIpc),
      diagnostics_(diagnostics), timing_(timing)
{
}

RemoteCommand::RemoteCommand(EventBus& bus, IpcClient& displayIpc, IpcClient& updateIpc,
                             const DiagnosticsUploader& diagnostics)
    : RemoteCommand(bus, displayIpc, updateIpc, diagnostics, Timing{})
{
}

void RemoteCommand::start()
{
    bus_.subscribe<Topic::REMOTE_COMMAND>([this](const nlohmann::json& msg)
    {
        std::lock_guard lk(mtx_);
        queue_.push_back(msg);
        cv_.notify_one();
    });
    Task::start();
}

void RemoteCommand::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        nlohmann::json msg;
        {
            std::unique_lock lock(mtx_);
            if (!cv_.wait(lock, st, [this] { return !queue_.empty(); }))
                return;
            msg = std::move(queue_.front());
            queue_.pop_front();
        }
        _handle(msg, st);
    }
}

void RemoteCommand::_handle(const nlohmann::json& msg, const std::stop_token& st)
{
    const auto id = msg.value("id", int64_t{0});
    const auto command = msg.value("command", std::string{});
    const auto arg = msg.value("arg", std::string{});
    const auto expiresAt = msg.value("expires_at", int64_t{0});
    logger_->info("Remote command #{} {} {}", id, command, arg);

    auto reject = [&](const std::string& reason)
    {
        logger_->warn("Remote command #{} rejected: {}", id, reason);
        _event(id, command, arg, "rejected (" + reason + ")");
        _reply(id, "rejected", reason);
    };

    const bool known = command == "request_diagnostics" || command == "restart_service" || isRebootClass(command);
    if (id <= 0 || !known)
        return reject("unknown command");
    if (command == "restart_service" ? !isKnownService(arg) : !arg.empty())
        return reject("invalid argument");
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (now > expiresAt)
        return reject("expired");

    if (isRebootClass(command) || (command == "restart_service" && arg == "update"))
    {
        if (const auto gate = _updateIdle(); !gate.ok)
            return reject(gate.reason);
    }
    if (command == "update_install")
    {
        const auto latest = updateIpc_.sendAndReceive({IpcMessageType::UpdateCheckLatest, {}});
        if (!latest || !latest->value("ok", false))
            return reject("update server not reachable");
        if (!(*latest)["release"].value("is_newer", false))
            return reject("No newer version available");
    }
    if (isRebootClass(command))
    {
        if (const auto gate = _safeToReboot(st); !gate.ok)
            return reject(gate.reason);

        const auto evidence = diagnostics_.captureBeforeReboot(id);
        if (!evidence.ok && command == "factory_reset")
            return reject("diagnostics upload failed, data would be lost: " + evidence.error);
        if (!evidence.ok)
            _event(id, command, arg, "diagnostics kept on device (" + evidence.error + ")");
    }

    _event(id, command, arg, "accepted");
    _reply(id, "accepted", "");

    if (command == "request_diagnostics")
    {
        const auto res = diagnostics_.collectAndUpload("admin", id);
        _reply(id, res.ok ? "done" : "failed", res.error);
        return;
    }
    if (command == "restart_service")
    {
        if (arg == "websocket")
            std::this_thread::sleep_for(timing_.settle);
        const auto res = Subprocess::run({"s6-svc", "-r", "/run/service/shareframe-" + arg}, 10);
        _reply(id, res.exitCode == 0 ? "done" : "failed", res.stdErr);
        return;
    }
    if (command == "update_install")
    {
        const auto res = updateIpc_.sendAndReceive({IpcMessageType::UpdatePerform, {}});
        if (!res || !res->value("ok", false))
        {
            const auto error = res ? res->value("error", std::string{"update did not start"}) : "update service unreachable";
            _event(id, command, arg, "failed (" + error + ")");
            _reply(id, "failed", error);
            return;
        }
        Subprocess::run({"shareframe-remote-gate", "mark"}, 5);
        return;
    }

    Subprocess::run({"shareframe-remote-gate", "mark"}, 5);
    std::this_thread::sleep_for(timing_.settle);
    const auto kind = command == "reboot" ? StatusScreenKind::Reboot : StatusScreenKind::FactoryReset;
    displayIpc_.send({IpcMessageType::ShowStatusScreen, {{"screen", toString(kind)}}});
    const auto res = command == "reboot"
        ? Subprocess::run({"reboot"}, 5)
        : Subprocess::run({"shareframe-factory-reset", "--yes"}, 5);
    if (res.exitCode != 0)
    {
        logger_->error("{} failed: {}", command, res.stdErr);
        displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
    }
}

RemoteCommand::Gate RemoteCommand::_updateIdle()
{
    // Unreachable update service = nothing installing; a reboot may be the fix.
    const auto res = updateIpc_.sendAndReceive({IpcMessageType::UpdateStatus, {}});
    if (!res)
        return {};
    const auto phase = (*res)["status"].value("phase", std::string{"idle"});
    if (phase != "idle" && phase != "failed")
        return {false, "update " + phase};
    return {};
}

RemoteCommand::Gate RemoteCommand::_safeToReboot(const std::stop_token& st)
{
    const auto board = Subprocess::run({"shareframe-remote-gate", "check"}, 5);
    if (board.exitCode != 0)
    {
        auto reason = board.stdOut;
        while (!reason.empty() && (reason.back() == '\n' || reason.back() == '\r'))
            reason.pop_back();
        return {false, reason.empty() ? "board gate failed" : reason};
    }

    const auto deadline = std::chrono::steady_clock::now() + timing_.refreshWait;
    while (!st.stop_requested())
    {
        const auto res = displayIpc_.sendAndReceive({IpcMessageType::GetSlideshowActive, {}});
        if (!res || !res->value("refreshing", false))
            return {};
        if (std::chrono::steady_clock::now() >= deadline)
            break;
        std::this_thread::sleep_for(timing_.refreshPoll);
    }
    return {false, "display still refreshing"};
}

void RemoteCommand::_reply(int64_t id, const std::string& status, const std::string& reason,
                           const nlohmann::json& result)
{
    bus_.publish<Topic::WS_SEND>({
        {"type", wsMessageTypeToString(WsMessageType::CommandResult)},
        {"id", id},
        {"status", status},
        {"reason", reason},
        {"result", result},
    });
}

void RemoteCommand::_event(int64_t id, const std::string& command, const std::string& arg, const std::string& text)
{
    std::string line = "#" + std::to_string(id) + " " + command;
    if (!arg.empty())
        line += " " + arg;
    Subprocess::run({"shareframe-event", "remote", line + " " + text}, 5);
}

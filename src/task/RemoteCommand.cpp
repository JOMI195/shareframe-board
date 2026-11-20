#include "task/RemoteCommand.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "net/WsProtocol.hpp"
#include "update/UpdatePhase.hpp"
#include "util/Subprocess.hpp"
#include <thread>

namespace
{
using remote::Command;

std::string trimmed(std::string s)
{
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
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
    Request request;
    int64_t expiresAt = 0;
    try
    {
        request = {msg.value("id", int64_t{0}), msg.value("command", std::string{}), msg.value("arg", std::string{})};
        expiresAt = msg.value("expires_at", int64_t{0});
    }
    catch (const nlohmann::json::exception& e)
    {
        logger_->warn("Dropping malformed remote command: {}", e.what());
        if (const auto id = msg.find("id"); id != msg.end() && id->is_number_integer())
            _reject({id->get<int64_t>(), {}, {}}, "malformed command");
        return;
    }
    logger_->info("Remote command #{} {} {}", request.id, request.name, request.arg);

    const auto command = remote::commandFromString(request.name);
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (const auto reason = remote::refusal(command, request.id, request.arg, now, expiresAt))
        return _reject(request, std::string(*reason));
    if (!_passesGates(request, *command, st))
        return;

    _event(request, "accepted");
    _reply(request.id, "accepted", "");
    _execute(request, *command);
}

bool RemoteCommand::_passesGates(const Request& request, const Command command, const std::stop_token& st)
{
    const auto refuse = [&](const std::string& reason)
    {
        _reject(request, reason);
        return false;
    };

    if (remote::needsIdleUpdate(command, request.arg))
    {
        if (const auto gate = _updateIdle(); !gate.ok)
            return refuse(gate.reason);
    }
    if (command == Command::UpdateInstall)
    {
        const auto latest = updateIpc_.sendAndReceive({IpcMessageType::UpdateCheckLatest, {}});
        if (!latest || !latest->value("ok", false))
            return refuse("update server not reachable");
        if (!(*latest)["release"].value("is_newer", false))
            return refuse("No newer version available");
    }
    if (remote::isRebootClass(command))
    {
        if (const auto gate = _safeToReboot(st, command != Command::UpdateInstall); !gate.ok)
            return refuse(gate.reason);

        const auto evidence = diagnostics_.captureBeforeReboot(request.id);
        if (!evidence.ok && command == Command::FactoryReset)
            return refuse("diagnostics upload failed, data would be lost: " + evidence.error);
        if (!evidence.ok)
            _event(request, "diagnostics kept on device (" + evidence.error + ")");
    }
    return true;
}

void RemoteCommand::_execute(const Request& request, const Command command)
{
    switch (command)
    {
    case Command::RequestDiagnostics: return _requestDiagnostics(request);
    case Command::RestartService: return _restartService(request);
    case Command::EnterSafeMode:
    case Command::ExitSafeMode: return _switchSafeMode(request, command);
    case Command::UpdateInstall: return _installUpdate(request);
    case Command::Reboot:
    case Command::FactoryReset:
    case Command::Shutdown: return _takeDown(request, command);
    }
}

void RemoteCommand::_requestDiagnostics(const Request& request)
{
    const auto res = diagnostics_.collectAndUpload("admin", request.id);
    _reply(request.id, res.ok ? "done" : "failed", res.error);
}

void RemoteCommand::_restartService(const Request& request)
{
    if (request.arg == "websocket")
        std::this_thread::sleep_for(timing_.settle);
    const auto res = Subprocess::run({"s6-svc", "-r", "/run/service/shareframe-" + request.arg}, 10);
    _reply(request.id, res.exitCode == 0 ? "done" : "failed", res.stdErr);
}

void RemoteCommand::_switchSafeMode(const Request& request, const Command command)
{
    const auto res = Subprocess::run({"shareframe-safe-mode", command == Command::EnterSafeMode ? "enter" : "exit"}, 15);
    if (res.exitCode == 0)
        return _reply(request.id, "done", trimmed(res.stdOut));
    const auto reason = trimmed(res.stdOut.empty() ? res.stdErr : res.stdOut);
    _event(request, "failed (" + reason + ")");
    _reply(request.id, "failed", reason);
}

void RemoteCommand::_installUpdate(const Request& request)
{
    const auto res = updateIpc_.sendAndReceive({IpcMessageType::UpdatePerform, {}});
    if (!res || !res->value("ok", false))
    {
        const auto error = res ? res->value("error", std::string{"update did not start"}) : "update service unreachable";
        _event(request, "failed (" + error + ")");
        _reply(request.id, "failed", error);
        return;
    }
    Subprocess::run({"shareframe-remote-gate", "mark"}, 5);
}

void RemoteCommand::_takeDown(const Request& request, const Command command)
{
    Subprocess::run({"shareframe-remote-gate", "mark"}, 5);
    std::this_thread::sleep_for(timing_.settle);
    // No status screen: the display blanks the panel when it stops, so no image burns in while the frame is off.
    if (command == Command::Shutdown)
    {
        if (const auto res = Subprocess::run({"shutdown"}, 5); res.exitCode != 0)
            logger_->error("shutdown failed: {}", res.stdErr);
        return;
    }
    const auto kind = command == Command::Reboot ? StatusScreenKind::Reboot : StatusScreenKind::FactoryReset;
    displayIpc_.send({IpcMessageType::ShowStatusScreen, {{"screen", toString(kind)}}});
    const auto res = command == Command::Reboot
        ? Subprocess::run({"reboot"}, 5)
        : Subprocess::run({"shareframe-factory-reset", "--yes"}, 5);
    if (res.exitCode != 0)
    {
        logger_->error("{} failed: {}", request.name, res.stdErr);
        displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
    }
}

void RemoteCommand::_reject(const Request& request, const std::string& reason)
{
    logger_->warn("Remote command #{} rejected: {}", request.id, reason);
    _event(request, "rejected (" + reason + ")");
    _reply(request.id, "rejected", reason);
}

RemoteCommand::Gate RemoteCommand::_updateIdle()
{
    // Unreachable update service = nothing installing; a reboot may be the fix.
    const auto res = updateIpc_.sendAndReceive({IpcMessageType::UpdateStatus, {}});
    if (!res)
        return {};
    const auto phase = (*res)["status"].value("phase", std::string{"idle"});
    if (const auto known = updatePhaseFromString(phase); !known || isBusy(*known))
        return {false, "update " + phase};
    return {};
}

RemoteCommand::Gate RemoteCommand::_safeToReboot(const std::stop_token& st, bool rateLimited)
{
    std::vector<std::string> gateCmd{"shareframe-remote-gate", "check"};
    if (!rateLimited)
        gateCmd.emplace_back("--no-rate-limit");
    const auto board = Subprocess::run(gateCmd, 5);
    if (board.exitCode != 0)
    {
        const auto reason = trimmed(board.stdOut);
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

void RemoteCommand::_event(const Request& request, const std::string& text)
{
    std::string line = "#" + std::to_string(request.id) + " " + request.name;
    if (!request.arg.empty())
        line += " " + request.arg;
    Subprocess::run({"shareframe-event", "remote", line + " " + text}, 5);
}

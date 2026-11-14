#pragma once
#include "net/DiagnosticsUploader.hpp"
#include "task/RemoteCommandRules.hpp"
#include "task/Task.hpp"
#include <chrono>
#include <deque>
#include <nlohmann/json.hpp>
#include <string>

class IpcClient;

/// Executes admin-triggered remote commands from the websocket. Only an
/// allowlist runs, each through the same paths the dashboard uses, after the
/// device confirms it is in a safe state. Results go back as command_result.
class RemoteCommand : public Task
{
public:
    struct Timing
    {
        std::chrono::milliseconds refreshWait{std::chrono::seconds(90)};
        std::chrono::milliseconds refreshPoll{std::chrono::seconds(2)};
        // Lets the accepted result leave the socket before the frame goes down.
        std::chrono::milliseconds settle{std::chrono::seconds(2)};
    };

    RemoteCommand(EventBus& bus, IpcClient& displayIpc, IpcClient& updateIpc,
                  const DiagnosticsUploader& diagnostics, Timing timing);
    RemoteCommand(EventBus& bus, IpcClient& displayIpc, IpcClient& updateIpc,
                  const DiagnosticsUploader& diagnostics);

    void start() override;

protected:
    void _run(std::stop_token st) override;

private:
    struct Gate
    {
        bool ok = true;
        std::string reason;
    };

    /// A command as it arrived; the name stays as sent for the replies and the event log.
    struct Request
    {
        int64_t id = 0;
        std::string name;
        std::string arg;
    };

    void _handle(const nlohmann::json& msg, const std::stop_token& st);
    /// The board's and the display's say before the frame goes down; a refusal is already answered.
    [[nodiscard]] bool _passesGates(const Request& request, remote::Command command, const std::stop_token& st);
    void _execute(const Request& request, remote::Command command);
    void _requestDiagnostics(const Request& request);
    void _restartService(const Request& request);
    void _switchSafeMode(const Request& request, remote::Command command);
    void _installUpdate(const Request& request);
    void _takeDown(const Request& request, remote::Command command);
    Gate _safeToReboot(const std::stop_token& st, bool rateLimited);
    Gate _updateIdle();
    void _reject(const Request& request, const std::string& reason);
    void _reply(int64_t id, const std::string& status, const std::string& reason,
                const nlohmann::json& result = nullptr);
    static void _event(const Request& request, const std::string& text);

    IpcClient& displayIpc_;
    IpcClient& updateIpc_;
    const DiagnosticsUploader& diagnostics_;
    Timing timing_;
    std::deque<nlohmann::json> queue_;
};

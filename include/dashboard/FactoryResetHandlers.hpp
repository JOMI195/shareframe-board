#pragma once
#include "dashboard/FactoryResetGuard.hpp"
#include "ipc/IpcClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <optional>
#include <spdlog/spdlog.h>

/// Factory reset from the dashboard, signed in or not: a code on the panel proves that whoever
/// asks stands at the frame. The server's remote reset does not come through here.
class FactoryResetHandlers
{
public:
    FactoryResetHandlers(IpcClient& displayIpc, IpcClient& updateIpc);

    /// Puts a reset code on the panel (or keeps the one already there).
    ix::HttpResponsePtr handleRequestCode(const ix::HttpRequestPtr& req);
    /// Resets with the code from the panel.
    ix::HttpResponsePtr handleReset(const ix::HttpRequestPtr& req);
    /// Takes the code back from the panel.
    ix::HttpResponsePtr handleCancel(const ix::HttpRequestPtr& req);

private:
    /// Why the frame must not reset right now; nothing when it may.
    [[nodiscard]] std::optional<std::string> _refusal() const;
    static void _event(const std::string& text);

    IpcClient& displayIpc_;
    IpcClient& updateIpc_;
    FactoryResetGuard guard_;
    std::shared_ptr<spdlog::logger> logger_;
};

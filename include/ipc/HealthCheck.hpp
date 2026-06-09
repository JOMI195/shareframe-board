#pragma once
#include "ipc/IpcClient.hpp"
#include "ipc/IpcProtocol.hpp"
#include <chrono>
#include <nlohmann/json.hpp>

/// Shared liveness primitives so every service exposes and queries health the
/// same way over nng. A service answering its REP endpoint at all means it is up.
namespace health
{
/// NngRepServer handler that answers get_health and acks everything else.
/// Use directly as the handler for services whose only IPC surface is health.
inline nlohmann::json respond(const IpcMessage& msg)
{
    if (msg.type == IpcMessageType::GetHealth)
        return {{"running", true}};
    return nlohmann::json::object();
}

/// Query a peer's health endpoint. True iff it replies running=true in time.
inline bool isRunning(IpcClient& client,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds{500})
{
    const auto resp = client.sendAndReceive({IpcMessageType::GetHealth, {}}, timeout);
    return resp.has_value() && resp->value("running", false);
}
} // namespace health

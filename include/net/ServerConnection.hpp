#pragma once
#include "ipc/IpcClient.hpp"
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>

/// The websocket's link to the Shareframe server as the other services see it.
struct ServerConnection
{
    bool connected = false;
    // Unix seconds the current online or offline period began; 0 = never connected.
    int64_t since = 0;
    int64_t lastContactAt = 0;
    bool operator==(const ServerConnection&) const = default;
};

inline nlohmann::json toJson(const ServerConnection& c)
{
    return {{"connected", c.connected}, {"since", c.since}, {"last_contact_at", c.lastContactAt}};
}

inline std::optional<ServerConnection> serverConnectionFromJson(const nlohmann::json& j)
{
    if (!j.is_object() || !j.contains("connected"))
        return std::nullopt;
    try
    {
        return ServerConnection{j.at("connected").get<bool>(), j.value("since", int64_t{0}),
                                j.value("last_contact_at", int64_t{0})};
    }
    catch (...)
    {
        return std::nullopt;
    }
}

/// Asks the websocket service; nullopt when it doesn't answer in time.
inline std::optional<ServerConnection> queryServerConnection(
    IpcClient& ws, const std::chrono::milliseconds timeout = std::chrono::milliseconds{300})
{
    const auto resp = ws.sendAndReceive({IpcMessageType::GetServerConnection, {}}, timeout);
    return resp ? serverConnectionFromJson(*resp) : std::nullopt;
}

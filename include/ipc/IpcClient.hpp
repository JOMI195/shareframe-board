#pragma once
#include "ipc/IpcProtocol.hpp"
#include <chrono>
#include <mutex>
#include <nng/nng.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

/// Request/response client over nng REQ. `url` is an nng endpoint
/// (e.g. "ipc:///tmp/shareframe-display.rep.sock"). The dialer reconnects
/// automatically, so the peer service may start after this client.
///
/// nng REQ/REP is strictly lockstep: every request expects a reply. `send()`
/// therefore performs a full round-trip and discards the reply body; use it for
/// fire-and-forget commands, `sendAndReceive()` for queries.
class IpcClient
{
public:
    explicit IpcClient(std::string url);
    ~IpcClient();

    bool connect();
    void disconnect();
    [[nodiscard]] bool isConnected() const;
    bool send(const IpcMessage& msg);
    std::optional<nlohmann::json> sendAndReceive(
        const IpcMessage& msg,
        std::chrono::milliseconds timeout = std::chrono::milliseconds{2000});

private:
    bool _ensure(); // caller must hold mtx_; opens socket + dials once

    std::string url_;
    std::shared_ptr<spdlog::logger> logger_;
    nng_socket sock_{};
    bool open_ = false;
    std::mutex mtx_;
};

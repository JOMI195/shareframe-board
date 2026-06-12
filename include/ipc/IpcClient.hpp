#pragma once
#include "ipc/IpcProtocol.hpp"
#include <chrono>
#include <mutex>
#include <nng/nng.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

/// Request/response client over nng REQ (auto-reconnecting, so the peer may
/// start later). REQ/REP is lockstep: send() does a full round-trip and discards
/// the reply (fire-and-forget); sendAndReceive() returns it.
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

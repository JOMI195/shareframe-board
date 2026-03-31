#pragma once
#include "ipc/IpcProtocol.hpp"
#include <chrono>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

class IpcClient
{
public:
    explicit IpcClient(const std::string& socketPath);
    ~IpcClient();

    bool connect();
    void disconnect();
    [[nodiscard]] bool isConnected() const;
    bool send(const IpcMessage& msg);
    std::optional<nlohmann::json> sendAndReceive(
        const IpcMessage& msg,
        std::chrono::milliseconds timeout = std::chrono::milliseconds{2000});

private:
    std::string socketPath_;
    std::shared_ptr<spdlog::logger> logger_;
    int fd_ = -1;
    std::mutex mtx_;
};

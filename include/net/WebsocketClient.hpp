#pragma once
#include "auth/AuthTokenManager.hpp"
#include "config/AppConfig.hpp"
#include "events/EventBus.hpp"
#include <atomic>
#include <ixwebsocket/IXWebSocket.h>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

class WebsocketClient
{
public:
    WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr);
    ~WebsocketClient();

    void start();
    void stop();

private:
    // ------- CONNECTION
    void _setupWebsocket();

    // ------- MESSAGE HANDLING
    void _processMessage(const std::string& text) const;

    // ------- RECONNECT
    void _handleReconnect(int closeCode);
    void _refreshToken();

    // ------- SEND QUEUE
    void _flushSendQueue();

    EventBus& bus_;
    const AppConfig& cfg_;
    AuthTokenManager& authMgr_;
    std::shared_ptr<spdlog::logger> logger_;
    ix::WebSocket ws_;
    std::atomic<int> reconnectCount_{0};
    std::jthread busThread_;
    std::mutex sendMtx_;
    std::vector<nlohmann::json> sendQueue_;
};

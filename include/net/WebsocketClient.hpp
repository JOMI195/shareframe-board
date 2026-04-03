#pragma once
#include "auth/AuthTokenManager.hpp"
#include "config/AppConfig.hpp"
#include "task/Task.hpp"
#include <atomic>
#include <ixwebsocket/IXWebSocket.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>

class WebsocketClient : public Task
{
public:
    WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr);

    void start() override;

protected:
    void _run(std::stop_token st) override;

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

    const AppConfig& cfg_;
    AuthTokenManager& authMgr_;
    ix::WebSocket ws_;
    std::atomic<int> reconnectCount_{0};
    std::mutex sendMtx_;
    std::vector<nlohmann::json> sendQueue_;
};

#pragma once
#include "auth/AuthTokenManager.hpp"
#include "config/AppConfig.hpp"
#include "fsm/Machine.hpp"
#include "net/ServerContact.hpp"
#include "net/ServerLink.hpp"
#include "task/Task.hpp"
#include <atomic>
#include <ixwebsocket/IXWebSocket.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>

class WebsocketClient : public Task
{
public:
    WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr, ServerContact& contact);

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
    /// Tells the display while the server refuses the frame because nobody owns it.
    void _reportIfUnregistered();
    void _onOpened();

    // ------- SEND QUEUE
    void _flushSendQueue();

    const AppConfig& cfg_;
    AuthTokenManager& authMgr_;
    ServerContact& contact_;
    ix::WebSocket ws_;
    std::atomic<int> reconnectCount_{0};
    fsm::Machine<ws::Link> link_;
    std::mutex sendMtx_;
    std::vector<nlohmann::json> sendQueue_;
};

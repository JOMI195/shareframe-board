#include "net/WebsocketClient.hpp"
#include "auth/TokenAuth.hpp"
#include "net/WsCloseCodes.hpp"
#include <nlohmann/json.hpp>

WebsocketClient::WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr)
    : bus_(bus), cfg_(cfg), authMgr_(authMgr), logger_(spdlog::default_logger()->clone("WebSocket"))
{
}

WebsocketClient::~WebsocketClient()
{
    stop();
}

// ------- CONNECTION

void WebsocketClient::_setupWebsocket()
{
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authMgr_);
    const ix::WebSocketHttpHeaders wsHeaders(authHeaders.begin(), authHeaders.end());

    auto url = cfg_.wsUrl();
    logger_->info("WebSocket URL: {}", url);
    ws_.setUrl(url);
    ws_.setExtraHeaders(wsHeaders);
    ws_.enableAutomaticReconnection();

    constexpr int pingIntervalSecs = 30;
    ws_.setPingInterval(pingIntervalSecs);

    constexpr int handshakeTimeoutSecs = 1;
    ws_.setHandshakeTimeout(handshakeTimeoutSecs);

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            logger_->info("WebSocket connected");
            reconnectCount_.store(0);
            _flushSendQueue();
        }
        else if (msg->type == ix::WebSocketMessageType::Close)
        {
            logger_->info("WebSocket disconnected: code={} reason={}", msg->closeInfo.code, msg->closeInfo.reason);
            _handleReconnect(msg->closeInfo.code);
        }
        else if (msg->type == ix::WebSocketMessageType::Error)
        {
            logger_->error("WebSocket error: http_status={} reason={}",
                           msg->errorInfo.http_status, msg->errorInfo.reason);
            _handleReconnect(0);
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            _processMessage(msg->str);
        }
    });
}

// ------- MESSAGE HANDLING

void WebsocketClient::_processMessage(const std::string& text) const
{
    logger_->debug("WS recv: {}", text);
    try
    {
        const auto json = nlohmann::json::parse(text);
        auto type = json.value("type", "");
        if (type.empty())
        {
            logger_->warn("Received message without type field");
            return;
        }

        // Dispatch by type string to typed publish.
        // Extend as server message types are defined:
        // if (type == "frame_update") {
        //     bus_.publish<Topic::FRAME_UPDATE>(msg::FrameUpdate{...});
        //     return;
        // }

        logger_->warn("Unhandled message type: {}", type);
    }
    catch (const nlohmann::json::parse_error&)
    {
        logger_->warn("Received non-JSON message: {}", text);
    }
}

// ------- RECONNECT

void WebsocketClient::_handleReconnect(int closeCode)
{
    if (closeCode == ws::CLOSE_AUTH_REJECTED || closeCode == ws::CLOSE_TOKEN_REVOKED)
    {
        logger_->info("Server rejected auth (code={}), refreshing token immediately", closeCode);
        reconnectCount_.store(0);
        _refreshToken();
        return;
    }

    if (constexpr int kTokenRefreshThreshold = 5; reconnectCount_.fetch_add(1) + 1 < kTokenRefreshThreshold)
        return;

    logger_->info("WebSocket reconnect threshold reached, refreshing token");
    reconnectCount_.store(0);
    _refreshToken();
}

void WebsocketClient::_refreshToken()
{
    authMgr_.invalidate();
    if (auto authHeaders = TokenAuth::buildTokenAuthHeaders(authMgr_); !authHeaders.empty())
    {
        ws_.setExtraHeaders(ix::WebSocketHttpHeaders(authHeaders.begin(), authHeaders.end()));
        logger_->info("Token refreshed, auto-reconnect will use new token");
    }
    else
    {
        logger_->error("Failed to obtain fresh token");
    }
}

// ------- SEND QUEUE

void WebsocketClient::_flushSendQueue()
{
    std::lock_guard lk(sendMtx_);
    for (const auto& msg : sendQueue_)
    {
        logger_->debug("WS send (queued): {}", msg.dump());
        ws_.send(msg.dump());
    }
    if (!sendQueue_.empty())
        logger_->info("Flushed {} queued messages", sendQueue_.size());
    sendQueue_.clear();
}

// ------- LIFECYCLE

void WebsocketClient::start()
{
    logger_->info("Starting WebSocket client");
    _setupWebsocket();
    ws_.start();

    bus_.subscribe<Topic::WS_SEND>([this](const nlohmann::json& msg)
    {
        std::lock_guard lk(sendMtx_);
        if (ws_.getReadyState() == ix::ReadyState::Open)
        {
            logger_->debug("WS send: {}", msg.dump());
            ws_.send(msg.dump());
        }
        else
        {
            logger_->debug("WS not connected, queuing message: {}", msg.value("type", "unknown"));
            sendQueue_.push_back(msg);
        }
    });

    busThread_ = std::jthread([this](const std::stop_token& st)
    {
        while (!st.stop_requested())
            bus_.waitAndProcess(st);
    });

    logger_->info("WebSocket client started");
}

void WebsocketClient::stop()
{
    logger_->info("Stopping WebSocket client");
    busThread_.request_stop();
    if (busThread_.joinable())
        busThread_.join();
    ws_.stop();
    logger_->info("WebSocket client stopped");
}

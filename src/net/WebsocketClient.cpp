#include "net/WebsocketClient.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "net/WsCloseCodes.hpp"
#include "net/WsProtocol.hpp"
#include <nlohmann/json.hpp>

WebsocketClient::WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr)
    : Task(bus, "WebSocket"), cfg_(cfg), authMgr_(authMgr)
{
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

    constexpr int handshakeTimeoutSecs = 60;
    ws_.setHandshakeTimeout(handshakeTimeoutSecs);

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            logger_->info("WebSocket connected");
            reconnectCount_.store(0);
            _flushSendQueue();
            bus_.publish<Topic::WS_CONNECTED>({});
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
    try
    {
        const auto json = nlohmann::json::parse(text);
        logger_->debug("WS recv: {}", logging::summarizePayloadForLog(text, "WS recv"));

        auto type = json.value("type", "");
        if (type.empty())
        {
            logger_->warn("Received message without type field");
            return;
        }

        const auto msgType = wsMessageTypeFromString(type);
        if (!msgType)
        {
            logger_->warn("Unhandled message type: {}", type);
            return;
        }

        switch (*msgType)
        {
        case WsMessageType::Picture:
            bus_.publish<Topic::PICTURE>(json);
            break;
        case WsMessageType::ClearImages:
            bus_.publish<Topic::CLEAR_IMAGES>(json);
            break;
        case WsMessageType::ClearDisplay:
            bus_.publish<Topic::CLEAR_DISPLAY>(json);
            break;
        }
    }
    catch (const nlohmann::json::parse_error&)
    {
        logger_->warn("Received non-JSON message: {}", logging::summarizePayloadForLog(text, "non-JSON"));
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
        const auto serialized = msg.dump();
        logger_->debug("WS send (queued): {}", logging::summarizePayloadForLog(serialized, "WS send"));
        ws_.send(serialized);
    }
    if (!sendQueue_.empty())
        logger_->info("Flushed {} queued messages", sendQueue_.size());
    sendQueue_.clear();
}

// ------- LIFECYCLE

void WebsocketClient::start()
{
    bus_.subscribe<Topic::WS_SEND>([this](const nlohmann::json& msg)
    {
        std::lock_guard lk(sendMtx_);
        if (ws_.getReadyState() == ix::ReadyState::Open)
        {
            const auto serialized = msg.dump();
            logger_->debug("WS send: {}", logging::summarizePayloadForLog(serialized, "WS send"));
            ws_.send(serialized);
        }
        else
        {
            logger_->debug("WS not connected, queuing message: {}", msg.value("type", "unknown"));
            sendQueue_.push_back(msg);
        }
    });

    Task::start();
}

void WebsocketClient::_run(const std::stop_token st)
{
    _setupWebsocket();
    ws_.start();
    logger_->info("WebSocket client started");

    {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, st, [] { return false; });
    }

    ws_.stop();
    logger_->info("WebSocket client stopped");
}

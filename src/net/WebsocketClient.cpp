#include "net/WebsocketClient.hpp"
#include "auth/TokenAuth.hpp"
#include "logging/LogSanitizer.hpp"
#include "net/WsProtocol.hpp"
#include <nlohmann/json.hpp>

WebsocketClient::WebsocketClient(EventBus& bus, const AppConfig& cfg, AuthTokenManager& authMgr,
                                 ServerContact& contact)
    : Task(bus, "WebSocket"), cfg_(cfg), authMgr_(authMgr), contact_(contact),
      link_(ws::Link::Connecting, logger_, spdlog::level::debug)
{
}

// ------- CONNECTION

void WebsocketClient::_setupWebsocket()
{
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authMgr_);
    const ix::WebSocketHttpHeaders wsHeaders(authHeaders.begin(), authHeaders.end());
    ws_.setMaxWaitBetweenReconnectionRetries(ws::maxReconnectWaitMs(link_.state()));
    _reportIfUnregistered();

    auto url = cfg_.wsUrl();
    logger_->info("WebSocket URL: {}", url);
    ws_.setUrl(url);
    ws_.setExtraHeaders(wsHeaders);
    ws_.enableAutomaticReconnection();

    constexpr uint32_t minReconnectWaitMs = 1000;
    ws_.setMinWaitBetweenReconnectionRetries(minReconnectWaitMs);

    constexpr int pingIntervalSecs = 30;
    ws_.setPingInterval(pingIntervalSecs);

    constexpr int handshakeTimeoutSecs = 60;
    ws_.setHandshakeTimeout(handshakeTimeoutSecs);

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            logger_->info("WebSocket connected");
            contact_.onOpen();
            reconnectCount_.store(0);
            downSinceOpen_.store(false);
            _onOpened();
            _flushSendQueue();
            bus_.publish<Topic::WS_CONNECTED>({});
        }
        else if (msg->type == ix::WebSocketMessageType::Close)
        {
            logger_->info("WebSocket disconnected: code={} reason={}", msg->closeInfo.code, msg->closeInfo.reason);
            downSinceOpen_.store(true);
            contact_.onClose();
            link_.moveTo(ws::onClosed(link_.state()));
            _handleReconnect(msg->closeInfo.code);
        }
        else if (msg->type == ix::WebSocketMessageType::Error)
        {
            logger_->error("WebSocket error: http_status={} reason={}",
                           msg->errorInfo.http_status, msg->errorInfo.reason);
            downSinceOpen_.store(true);
            contact_.onClose();
            link_.moveTo(ws::onClosed(link_.state()));
            // The server refuses a handshake it never accepted with 403 instead of a close code.
            const int status = msg->errorInfo.http_status;
            _handleReconnect(status == 401 || status == 403 ? ws::CLOSE_AUTH_REJECTED : 0);
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            contact_.touch();
            _processMessage(msg->str);
        }
        else if (msg->type == ix::WebSocketMessageType::Pong)
        {
            contact_.touch();
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
        if (!json.is_object())
        {
            logger_->warn("Received a message that is not an object: {}", logging::summarizePayloadForLog(text, "WS recv"));
            return;
        }

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
        case WsMessageType::Command:
            bus_.publish<Topic::REMOTE_COMMAND>(json);
            break;
        default:
            logger_->warn("Unexpected inbound message type: {}", type);
            break;
        }
    }
    catch (const nlohmann::json::parse_error&)
    {
        logger_->warn("Received non-JSON message: {}", logging::summarizePayloadForLog(text, "non-JSON"));
    }
    catch (const nlohmann::json::exception& e)
    {
        logger_->warn("Received malformed message ({}): {}", e.what(), logging::summarizePayloadForLog(text, "WS recv"));
    }
}

// ------- RECONNECT

void WebsocketClient::_handleReconnect(int closeCode)
{
    const auto action = ws::tokenAction(closeCode, reconnectCount_.fetch_add(1) + 1);
    if (action == ws::TokenAction::Keep)
        return;

    if (action == ws::TokenAction::Replace)
    {
        logger_->info("Server rejected auth (code={}), refreshing token immediately", closeCode);
        authMgr_.invalidate();
    }
    else
        logger_->info("WebSocket reconnect threshold reached, reloading token");
    reconnectCount_.store(0);
    _applyToken();
}

void WebsocketClient::_applyToken()
{
    if (auto authHeaders = TokenAuth::buildTokenAuthHeaders(authMgr_); !authHeaders.empty())
    {
        ws_.setExtraHeaders(ix::WebSocketHttpHeaders(authHeaders.begin(), authHeaders.end()));
        logger_->info("Token loaded, auto-reconnect will use it");
    }
    else
    {
        logger_->error("Failed to obtain a token");
        _reportIfUnregistered();
    }
}

void WebsocketClient::_reportIfUnregistered()
{
    if (!authMgr_.isUnregistered())
        return;
    const auto next = ws::onRefusedAsUnowned(link_.state());
    if (next != ws::Link::Unregistered)
        return;
    link_.moveTo(next);
    ws_.setMaxWaitBetweenReconnectionRetries(ws::maxReconnectWaitMs(next));
    bus_.publish<Topic::FRAME_REGISTRATION>({false, authMgr_.claimCode()});
}

void WebsocketClient::_onOpened()
{
    const bool wasUnregistered = link_.state() == ws::Link::Unregistered;
    link_.moveTo(ws::onOpened(link_.state()));
    if (!wasUnregistered)
        return;
    ws_.setMaxWaitBetweenReconnectionRetries(ws::maxReconnectWaitMs(link_.state()));
    bus_.publish<Topic::FRAME_REGISTRATION>({true});
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

void WebsocketClient::reconnectNow()
{
    std::lock_guard lk(restartMtx_);
    if (stopping_ || !downSinceOpen_.load() || ws_.getReadyState() == ix::ReadyState::Open)
        return;
    // The socket keeps its retry count until it connects; only a restart begins at the shortest wait.
    logger_->info("Reconnecting WebSocket now");
    ws_.stop();
    ws_.start();
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

    {
        std::lock_guard lk(restartMtx_);
        stopping_ = true;
    }
    ws_.stop();
    logger_->info("WebSocket client stopped");
}

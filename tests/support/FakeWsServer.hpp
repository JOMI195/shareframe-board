#pragma once
#include <ixwebsocket/IXWebSocketServer.h>
#include <mutex>
#include <random>
#include <string>
#include <vector>

/// Local WebSocket server standing in for the Shareframe frames socket.
class FakeWsServer
{
public:
    FakeWsServer()
    {
        std::random_device rd;
        port_ = 20000 + static_cast<int>(rd() % 30000);
        server_ = std::make_unique<ix::WebSocketServer>(port_, "127.0.0.1");
        server_->setOnClientMessageCallback(
            [this](const std::shared_ptr<ix::ConnectionState>&, ix::WebSocket&, const ix::WebSocketMessagePtr& msg)
            {
                std::lock_guard lk(mtx_);
                if (msg->type == ix::WebSocketMessageType::Open)
                    openHeaders_.push_back(msg->openInfo.headers);
                else if (msg->type == ix::WebSocketMessageType::Message)
                    received_.push_back(msg->str);
            });
    }

    ~FakeWsServer() { server_->stop(); }

    bool start() { return server_->listenAndStart(); }

    void sendToAll(const std::string& text)
    {
        for (const auto& client : server_->getClients())
            client->send(text);
    }

    [[nodiscard]] size_t clientCount() { return server_->getClients().size(); }

    [[nodiscard]] std::vector<ix::WebSocketHttpHeaders> openHeaders() const
    {
        std::lock_guard lk(mtx_);
        return openHeaders_;
    }

    [[nodiscard]] std::vector<std::string> received() const
    {
        std::lock_guard lk(mtx_);
        return received_;
    }

    [[nodiscard]] std::string baseUrl() const { return "127.0.0.1:" + std::to_string(port_); }

private:
    int port_ = 0;
    std::unique_ptr<ix::WebSocketServer> server_;
    mutable std::mutex mtx_;
    std::vector<ix::WebSocketHttpHeaders> openHeaders_;
    std::vector<std::string> received_;
};

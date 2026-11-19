#pragma once
#include <ixwebsocket/IXHttpServer.h>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <vector>

/// Local server standing in for the Shareframe frames socket, with canned HTTP answers per
/// "METHOD /path" on the same port, as the frame reaches both through one base URL.
class FakeWsServer
{
public:
    FakeWsServer()
    {
        std::random_device rd;
        port_ = 20000 + static_cast<int>(rd() % 30000);
        server_ = std::make_unique<ix::HttpServer>(port_, "127.0.0.1");
        server_->setOnConnectionCallback([this](const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>&)
        {
            std::lock_guard lk(mtx_);
            const auto path = req->uri.substr(0, req->uri.find('?'));
            ++httpRequests_[req->method + " " + path];
            const auto it = routes_.find(req->method + " " + path);
            const auto [status, body] = it != routes_.end() ? it->second : std::pair{404, std::string("{}")};
            ix::WebSocketHttpHeaders headers{{"Content-Type", "application/json"}};
            return std::make_shared<ix::HttpResponse>(status, "", ix::HttpErrorCode::Ok, headers, body);
        });
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

    void closeAll(const int code, const std::string& reason = "")
    {
        for (const auto& client : server_->getClients())
            client->close(static_cast<uint16_t>(code), reason);
    }

    void on(const std::string& methodAndPath, int status, std::string body = "{}")
    {
        std::lock_guard lk(mtx_);
        routes_[methodAndPath] = {status, std::move(body)};
    }

    [[nodiscard]] int httpRequests(const std::string& methodAndPath) const
    {
        std::lock_guard lk(mtx_);
        const auto it = httpRequests_.find(methodAndPath);
        return it != httpRequests_.end() ? it->second : 0;
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

    [[nodiscard]] int port() const { return port_; }
    [[nodiscard]] std::string baseUrl() const { return "127.0.0.1:" + std::to_string(port_); }

private:
    int port_ = 0;
    std::unique_ptr<ix::HttpServer> server_;
    mutable std::mutex mtx_;
    std::vector<ix::WebSocketHttpHeaders> openHeaders_;
    std::vector<std::string> received_;
    std::map<std::string, std::pair<int, std::string>> routes_;
    std::map<std::string, int> httpRequests_;
};

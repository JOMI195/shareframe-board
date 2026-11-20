#pragma once
#include <functional>
#include <ixwebsocket/IXHttpServer.h>
#include <map>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

/// Local HTTP server standing in for the Shareframe backend: canned answers per
/// "METHOD /path", every request recorded.
class FakeHttpServer
{
public:
    struct Request
    {
        std::string method;
        std::string path;
        std::string body;
        ix::WebSocketHttpHeaders headers;
        std::string uri;
    };

    FakeHttpServer()
    {
        std::random_device rd;
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            port_ = 20000 + static_cast<int>(rd() % 30000);
            server_ = std::make_unique<ix::HttpServer>(port_, "127.0.0.1");
            server_->setOnConnectionCallback([this](const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>&)
            {
                return _answer(req);
            });
            if (server_->listen().first)
            {
                server_->start();
                return;
            }
        }
        throw std::runtime_error("FakeHttpServer: no free port");
    }

    ~FakeHttpServer() { server_->stop(); }

    void on(const std::string& methodAndPath, int status, std::string body = "{}")
    {
        std::lock_guard lk(mtx_);
        routes_[methodAndPath] = {status, std::move(body)};
    }

    void redirect(const std::string& methodAndPath, const std::string& location)
    {
        std::lock_guard lk(mtx_);
        routes_[methodAndPath] = {302, "{}"};
        locations_[methodAndPath] = location;
    }

    /// Runs while the server answers "METHOD /path", before the reply goes out: what else happens meanwhile.
    void during(const std::string& methodAndPath, std::function<void()> action)
    {
        std::lock_guard lk(mtx_);
        during_[methodAndPath] = std::move(action);
    }

    [[nodiscard]] std::vector<Request> requests(const std::string& path = "") const
    {
        std::lock_guard lk(mtx_);
        std::vector<Request> out;
        for (const auto& r : requests_)
            if (path.empty() || r.path == path)
                out.push_back(r);
        return out;
    }

    [[nodiscard]] int port() const { return port_; }
    /// The AppConfig::baseUrl form (host:port, scheme comes from `production`).
    [[nodiscard]] std::string baseUrl() const { return "127.0.0.1:" + std::to_string(port_); }

private:
    ix::HttpResponsePtr _answer(const ix::HttpRequestPtr& req)
    {
        std::lock_guard lk(mtx_);
        const auto path = req->uri.substr(0, req->uri.find('?'));
        requests_.push_back({req->method, path, req->body, req->headers, req->uri});
        if (const auto action = during_.find(req->method + " " + path); action != during_.end())
            action->second();
        const auto it = routes_.find(req->method + " " + path);
        const auto [status, body] = it != routes_.end() ? it->second : std::pair{404, std::string("{}")};
        ix::WebSocketHttpHeaders headers{{"Content-Type", "application/json"}};
        if (const auto location = locations_.find(req->method + " " + path); location != locations_.end())
            headers["Location"] = location->second;
        return std::make_shared<ix::HttpResponse>(status, "", ix::HttpErrorCode::Ok, headers, body);
    }

    int port_ = 0;
    std::unique_ptr<ix::HttpServer> server_;
    mutable std::mutex mtx_;
    std::map<std::string, std::pair<int, std::string>> routes_;
    std::map<std::string, std::function<void()>> during_;
    std::map<std::string, std::string> locations_;
    std::vector<Request> requests_;
};

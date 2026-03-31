#pragma once
#include "config/AppConfig.hpp"
#include "dashboard/SessionManager.hpp"
#include "ipc/IpcClient.hpp"
#include "net/HTTPClient.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>

class DashboardServer
{
public:
    DashboardServer(const AppConfig& cfg, IpcClient& ipc,
                    HTTPClient& http, SessionManager& sessions);

    void start();
    void stop();

private:
    ix::HttpResponsePtr _handleRequest(ix::HttpRequestPtr req,
                                       std::shared_ptr<ix::ConnectionState> connState);

    // Auth endpoints
    ix::HttpResponsePtr _handleLogin(const ix::HttpRequestPtr& req);
    ix::HttpResponsePtr _handleCheckAuth(const ix::HttpRequestPtr& req);
    ix::HttpResponsePtr _handleLogout(const ix::HttpRequestPtr& req);

    // Slideshow endpoints
    ix::HttpResponsePtr _handleSkipImage(const ix::HttpRequestPtr& req);
    ix::HttpResponsePtr _handleGetInterval(const ix::HttpRequestPtr& req);
    ix::HttpResponsePtr _handleUpdateInterval(const ix::HttpRequestPtr& req);

    // Helpers
    ix::HttpResponsePtr _jsonResponse(int status, const std::string& statusText,
                                      const nlohmann::json& body);
    ix::HttpResponsePtr _loginRequired(const ix::HttpRequestPtr& req);
    std::string _extractSessionId(const ix::HttpRequestPtr& req);

    const AppConfig& cfg_;
    IpcClient& ipc_;
    HTTPClient& http_;
    SessionManager& sessions_;
    ix::HttpServer server_;
    std::shared_ptr<spdlog::logger> logger_;
};

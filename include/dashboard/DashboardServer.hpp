#pragma once
#include "config/AppConfig.hpp"
#include "dashboard/FrameHandlers.hpp"
#include "dashboard/LoginThrottle.hpp"
#include "dashboard/ServiceHandlers.hpp"
#include "dashboard/SessionManager.hpp"
#include "dashboard/SystemHandlers.hpp"
#include "dashboard/UpdateHandlers.hpp"
#include "dashboard/WifiHandlers.hpp"
#include "net/HTTPClient.hpp"
#include "repository/SettingsRepository.hpp"
#include <functional>
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

class AuthTokenManager;
class IpcClient;
class WifiManager;

class DashboardServer
{
public:
    DashboardServer(AppConfig& cfg, IpcClient& ipc, HTTPClient& http,
                    SessionManager& sessions, AuthTokenManager& authTokenManager,
                    WifiManager& wifi, IpcClient& updateIpc,
                    SettingsRepository& settings);

    void start();
    void stop();

private:
    using QueryParams = std::unordered_map<std::string, std::string>;
    using RouteHandler = std::function<ix::HttpResponsePtr(
        const ix::HttpRequestPtr&, const QueryParams&)>;

    struct Route
    {
        std::string method;
        std::string path;
        RouteHandler handler;
    };

    void _initRoutes();

    ix::HttpResponsePtr _handleRequest(const ix::HttpRequestPtr& req,
                                       const std::shared_ptr<ix::ConnectionState>& connState) const;

    // Auth endpoints
    ix::HttpResponsePtr _handleLogin(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handlePasswordLogin(const std::string& password) const;
    ix::HttpResponsePtr _handleChangePassword(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handleCheckAuth(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handleLogout(const ix::HttpRequestPtr& req) const;

    // ap_password is only included for AP clients or an authenticated session.
    ix::HttpResponsePtr _handleConnectionMode(const ix::HttpRequestPtr& req) const;

    // Helpers
    ix::HttpResponsePtr _loginRequired(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _sessionLoginResponse() const;
    // True when the password matches the stored hash, or the initial password if none set.
    bool _verifyDashboardPassword(const std::string& password) const;
    bool _isPasswordLoginConfigured() const;
    bool _hasValidSession(const ix::HttpRequestPtr& req) const;
    static std::string _extractSessionId(const ix::HttpRequestPtr& req);
    // True while the board hosts its AP fallback (OTP login impossible offline).
    static bool _isApMode();

    AppConfig& cfg_;
    HTTPClient& http_;
    SessionManager& sessions_;
    AuthTokenManager& authTokenManager_;
    SettingsRepository& settings_;
    WifiManager& wifi_;
    mutable LoginThrottle throttle_;
    ix::HttpServer server_;
    std::shared_ptr<spdlog::logger> logger_;

    // Handler classes
    WifiHandlers wifiHandlers_;
    FrameHandlers frameHandlers_;
    SystemHandlers systemHandlers_;
    ServiceHandlers serviceHandlers_;
    UpdateHandlers updateHandlers_;

    // Routing tables
    std::vector<Route> publicRoutes_;
    std::vector<Route> protectedRoutes_;
};

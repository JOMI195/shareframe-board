#pragma once
#include "config/AppConfig.hpp"
#include "dashboard/FactoryResetHandlers.hpp"
#include "dashboard/FrameHandlers.hpp"
#include "dashboard/SlideshowImageHandlers.hpp"
#include "dashboard/DefaultImageHandlers.hpp"
#include "dashboard/LoginThrottle.hpp"
#include "dashboard/ServiceHandlers.hpp"
#include "dashboard/SessionManager.hpp"
#include "dashboard/SystemHandlers.hpp"
#include "dashboard/UpdateHandlers.hpp"
#include "dashboard/WifiHandlers.hpp"
#include "net/DiagnosticsUploader.hpp"
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
                    WifiManager& wifi, IpcClient& updateIpc, IpcClient& wsIpc,
                    SettingsRepository& settings, IImageRepository& images);

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
    ix::HttpResponsePtr _dispatch(const ix::HttpRequestPtr& req) const;

    // Auth endpoints
    ix::HttpResponsePtr _handleLogin(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handleOtpLogin(const ix::HttpRequestPtr& req, const std::string& otp) const;
    ix::HttpResponsePtr _handleSetupCodeLogin(const ix::HttpRequestPtr& req, const std::string& code) const;
    ix::HttpResponsePtr _handlePasswordLogin(const ix::HttpRequestPtr& req, const std::string& password) const;
    ix::HttpResponsePtr _handleChangePassword(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handleCheckAuth(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _handleLogout(const ix::HttpRequestPtr& req) const;

    // ap_password is only included for AP clients or an authenticated session.
    ix::HttpResponsePtr _handleConnectionMode(const ix::HttpRequestPtr& req) const;

    // Helpers
    ix::HttpResponsePtr _loginRequired(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr _sessionLoginResponse(SessionOrigin origin) const;
    // Password sessions confirm the current password before changing it.
    bool _needsCurrentPassword(const std::string& sessionId) const;
    // A browser's cross-site request names another origin than the host it was sent to.
    static bool _sameOrigin(const ix::HttpRequestPtr& req);
    static std::string _client(const ix::HttpRequestPtr& req);
    static ix::HttpResponsePtr _throttled(const LoginThrottle::Attempt& attempt);
    // True when the password matches the stored hash, or the initial password if none set.
    bool _verifyDashboardPassword(const std::string& password) const;
    bool _isPasswordLoginConfigured() const;
    bool _hasValidSession(const ix::HttpRequestPtr& req) const;
    static std::string _extractSessionId(const ix::HttpRequestPtr& req);

    AppConfig& cfg_;
    HTTPClient& http_;
    SessionManager& sessions_;
    AuthTokenManager& authTokenManager_;
    SettingsRepository& settings_;
    WifiManager& wifi_;
    mutable LoginThrottle throttle_;
    // Sign-in waits on this call: no retries, so it gives up long before nginx does.
    HTTPClient otpHttp_{5, 8, 0};
    ix::HttpServer server_;
    std::shared_ptr<spdlog::logger> logger_;

    DiagnosticsUploader diagnostics_;
    DefaultImages defaultImages_;

    // Handler classes
    WifiHandlers wifiHandlers_;
    FrameHandlers frameHandlers_;
    SlideshowImageHandlers slideshowImageHandlers_;
    DefaultImageHandlers defaultImageHandlers_;
    SystemHandlers systemHandlers_;
    ServiceHandlers serviceHandlers_;
    UpdateHandlers updateHandlers_;
    FactoryResetHandlers factoryResetHandlers_;

    // Routing tables
    std::vector<Route> publicRoutes_;
    std::vector<Route> protectedRoutes_;
};

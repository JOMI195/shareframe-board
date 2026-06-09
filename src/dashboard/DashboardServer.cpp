#include "dashboard/DashboardServer.hpp"
#include "auth/TokenAuth.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "util/HttpUtil.hpp"
#include <fstream>

using dashboard::errorResponse;
using dashboard::jsonResponse;

DashboardServer::DashboardServer(AppConfig& cfg, IpcClient& ipc,
                                 HTTPClient& http, SessionManager& sessions,
                                 AuthTokenManager& authTokenManager, WifiManager& wifi)
    : cfg_(cfg), http_(http), sessions_(sessions),
      authTokenManager_(authTokenManager),
      server_(cfg.dashboardApplication.port, cfg.dashboardApplication.host),
      logger_(spdlog::default_logger()->clone("DashboardServer")),
      wifiHandlers_(wifi),
      frameHandlers_(ipc, cfg),
      systemHandlers_(cfg, http, authTokenManager, wifi),
      serviceHandlers_(cfg)
{
    _initRoutes();
}

void DashboardServer::start()
{
    server_.setOnConnectionCallback(
        [this](const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>& connState)
        {
            return _handleRequest(req, connState);
        });

    if (auto [ok, errMsg] = server_.listen(); !ok)
    {
        logger_->error("Failed to listen on {}:{}: {}", cfg_.dashboardApplication.host, cfg_.dashboardApplication.port,
                       errMsg);
        return;
    }

    server_.start();
    logger_->info("DashboardServer listening on {}:{}", cfg_.dashboardApplication.host, cfg_.dashboardApplication.port);
}

void DashboardServer::stop()
{
    server_.stop();
    logger_->info("DashboardServer stopped");
}

// --- Routing ---

void DashboardServer::_initRoutes()
{
    publicRoutes_ = {
        {"POST", "/api/auth/login",      [this](auto& req, auto&) { return _handleLogin(req); }},
        {"GET",  "/api/auth/status",      [this](auto& req, auto&) { return _handleCheckAuth(req); }},
        {"POST", "/api/auth/logout",      [this](auto& req, auto&) { return _handleLogout(req); }},
        {"GET",  "/api/system/health",   [this](auto& req, auto&) { return systemHandlers_.handleHealth(req); }},
        // Public so the SPA can read the network mode before any login and
        // route to the offline AP-setup view when the board is hosting its AP.
        {"GET",  "/api/connection/mode", [this](auto& req, auto&) { return wifiHandlers_.handleMode(req); }},
    };

    protectedRoutes_ = {
        // Frame / Slideshow
        {"GET",  "/api/frame/slideshow/status",   [this](auto& req, auto&) { return frameHandlers_.handleStatus(req); }},
        {"POST", "/api/frame/slideshow",          [this](auto& req, auto&) { return frameHandlers_.handleControl(req); }},
        {"POST", "/api/frame/slideshow/interval", [this](auto& req, auto&) { return frameHandlers_.handleUpdateInterval(req); }},
        {"POST", "/api/frame/slideshow/skip",     [this](auto& req, auto&) { return frameHandlers_.handleSkip(req); }},
        {"POST", "/api/frame/clear",              [this](auto& req, auto&) { return frameHandlers_.handleClear(req); }},
        {"GET",  "/api/frame/display/stats",      [this](auto& req, auto&) { return frameHandlers_.handleDisplayStats(req); }},

        // WiFi / Connection
        {"GET",  "/api/connection/status",         [this](auto& req, auto&) { return wifiHandlers_.handleStatus(req); }},
        {"GET",  "/api/connection/saved-networks", [this](auto& req, auto&) { return wifiHandlers_.handleSavedNetworks(req); }},
        {"POST", "/api/connection/connect",        [this](auto& req, auto&) { return wifiHandlers_.handleConnect(req); }},
        {"POST", "/api/connection/forget",         [this](auto& req, auto&) { return wifiHandlers_.handleForget(req); }},

        // System
        {"GET",  "/api/system/info",             [this](auto& req, auto&) { return systemHandlers_.handleInfo(req); }},
        {"GET",  "/api/system/check-internet",   [this](auto& req, auto&) { return systemHandlers_.handleCheckInternet(req); }},
        {"POST", "/api/system/restart",          [this](auto& req, auto&) { return systemHandlers_.handleRestart(req); }},
        {"POST", "/api/system/shutdown",         [this](auto& req, auto&) { return systemHandlers_.handleShutdown(req); }},
        {"GET",  "/api/system/logs",             [this](auto& req, auto& qp) { return systemHandlers_.handleLogs(req, qp); }},
        {"GET",  "/api/system/updates/latest",   [this](auto& req, auto&) { return systemHandlers_.handleLatestUpdate(req); }},

        // Service management
        {"GET",  "/api/services",                [this](auto& req, auto&) { return serviceHandlers_.handleList(req); }},
        {"POST", "/api/services/restart",        [this](auto& req, auto&) { return serviceHandlers_.handleRestart(req); }},
    };
}

ix::HttpResponsePtr DashboardServer::_handleRequest(
    const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>& connState) const
{
    auto [path, queryParams] = HttpUtil::parseUri(req->uri);

    logger_->debug("{} {} from {}:{}", req->method, path,
                   connState->getRemoteIp(), connState->getRemotePort());

    // Public routes (no auth required)
    for (const auto& [method, routePath, handler] : publicRoutes_)
        if (req->method == method && path == routePath)
            return handler(req, queryParams);

    // AP fallback: with no internet the user cannot complete the upstream OTP
    // login, so the connection endpoints (status/saved-networks/connect/forget)
    // are reachable without a session *only* while hosting the AP. Everything
    // else still requires auth.
    const bool apBypass = _isApMode() && path.rfind("/api/connection/", 0) == 0;

    // All remaining routes require auth
    if (!apBypass)
        if (auto denied = _loginRequired(req))
            return denied;

    // Protected routes
    for (const auto& [method, routePath, handler] : protectedRoutes_)
        if (req->method == method && path == routePath)
            return handler(req, queryParams);

    return errorResponse(404, "Not Found", "Not found");
}

// --- Auth Endpoints ---

ix::HttpResponsePtr DashboardServer::_handleLogin(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try
    {
        body = nlohmann::json::parse(req->body);
    }
    catch (...)
    {
        return errorResponse(400, "Bad Request", "Invalid JSON");
    }

    auto otpIt = body.find("otp");
    if (otpIt == body.end() || !otpIt->is_string())
        return errorResponse(400, "Bad Request", "Missing otp field");

    auto otp = otpIt->get<std::string>();

    // Authenticate the board→server request with the access token
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authTokenManager_);

    // POST to server's OTP verification endpoint
    nlohmann::json otpPayload = {{"otp", otp}};
    HTTPClient::Headers headers;
    for (auto& [k, v] : authHeaders)
        headers[k] = v;
    headers["Content-Type"] = "application/json";

    std::string url = cfg_.httpBaseUrl() + cfg_.dashboardApplication.httpVerifyOtpUrl;
    auto resp = http_.post(url, otpPayload.dump(), headers);

    if (!resp.ok())
    {
        logger_->warn("OTP verification request failed: {} {}", resp.statusCode, resp.errorMsg);
        return errorResponse(401, "Unauthorized", "OTP verification failed");
    }

    // Parse server response and read the plain valid flag
    nlohmann::json serverResp;
    try
    {
        serverResp = nlohmann::json::parse(resp.body);
    }
    catch (...)
    {
        logger_->error("Failed to parse OTP server response");
        return errorResponse(500, "Internal Server Error", "Internal error");
    }

    if (!serverResp.value("valid", false))
    {
        return errorResponse(401, "Unauthorized", "OTP invalid");
    }

    // Create session
    std::string sessionId = sessions_.createSession();
    logger_->info("Login successful, created session");

    ix::WebSocketHttpHeaders respHeaders;
    respHeaders["Content-Type"] = "application/json";
    respHeaders["Set-Cookie"] = "session=" + sessionId
        + "; HttpOnly; SameSite=Lax; Path=/; Max-Age=604800";

    // Same {success,message,data} envelope as jsonResponse, but built by hand
    // because we also need to attach the Set-Cookie header.
    nlohmann::json respBody = {
        {"success", true},
        {"message", "Login erfolgreich"},
        {"data", nullptr},
    };
    return std::make_shared<ix::HttpResponse>(
        200, "OK", ix::HttpErrorCode::Ok, respHeaders, respBody.dump());
}

ix::HttpResponsePtr DashboardServer::_handleCheckAuth(const ix::HttpRequestPtr& req) const
{
    const std::string sessionId = _extractSessionId(req);
    bool authenticated = !sessionId.empty() && sessions_.isValid(sessionId);
    return jsonResponse(200, "OK", {{"authenticated", authenticated}});
}

ix::HttpResponsePtr DashboardServer::_handleLogout(const ix::HttpRequestPtr& req) const
{
    if (const std::string sessionId = _extractSessionId(req); !sessionId.empty())
        sessions_.removeSession(sessionId);
    return jsonResponse(200, "OK", nlohmann::json::object());
}

// --- Helpers ---

bool DashboardServer::_isApMode()
{
    // Single word written by the wifi-mode-manager daemon: connecting|connected|ap.
    std::ifstream f("/run/shareframe/wifi-mode");
    std::string mode;
    if (f)
        std::getline(f, mode);
    return mode == "ap";
}

ix::HttpResponsePtr DashboardServer::_loginRequired(const ix::HttpRequestPtr& req) const
{
    if (const std::string sessionId = _extractSessionId(req); sessionId.empty() || !sessions_.isValid(sessionId))
    {
        return errorResponse(401, "Unauthorized", "Authentication required");
    }
    return nullptr; // authenticated
}

std::string DashboardServer::_extractSessionId(const ix::HttpRequestPtr& req)
{
    const auto it = req->headers.find("cookie");
    if (it == req->headers.end())
        return {};

    // Parse "session=<id>" from cookie header
    const std::string& cookie = it->second;
    const std::string prefix = "session=";
    const size_t pos = cookie.find(prefix);
    if (pos == std::string::npos)
        return {};

    const size_t start = pos + prefix.size();
    size_t end = cookie.find(';', start);
    if (end == std::string::npos)
        end = cookie.size();

    return cookie.substr(start, end - start);
}

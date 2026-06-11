#include "dashboard/DashboardServer.hpp"
#include "auth/PasswordHash.hpp"
#include "auth/TokenAuth.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "util/HttpUtil.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

using dashboard::errorResponse;
using dashboard::jsonResponse;

namespace
{
constexpr const char* kPasswordHashKey = "dashboard_password_hash";
const std::string kNotConfiguredMsg =
    "Passwort-Anmeldung ist auf diesem Gerät nicht eingerichtet.";
}

DashboardServer::DashboardServer(AppConfig& cfg, IpcClient& ipc,
                                 HTTPClient& http, SessionManager& sessions,
                                 AuthTokenManager& authTokenManager, WifiManager& wifi,
                                 IpcClient& updateIpc, SettingsRepository& settings)
    : cfg_(cfg), http_(http), sessions_(sessions),
      authTokenManager_(authTokenManager),
      settings_(settings), wifi_(wifi),
      server_(cfg.dashboardApplication.port, cfg.dashboardApplication.host),
      logger_(spdlog::default_logger()->clone("DashboardServer")),
      wifiHandlers_(wifi),
      frameHandlers_(ipc, cfg),
      systemHandlers_(cfg, wifi),
      serviceHandlers_(cfg),
      updateHandlers_(updateIpc)
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
        {"GET",  "/api/connection/mode", [this](auto& req, auto&) { return _handleConnectionMode(req); }},
    };

    protectedRoutes_ = {
        // Auth
        {"POST", "/api/auth/change-password", [this](auto& req, auto&) { return _handleChangePassword(req); }},

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
        {"POST", "/api/connection/ap-password",    [this](auto& req, auto&) { return wifiHandlers_.handleSetApPassword(req); }},

        // System
        {"GET",  "/api/system/info",             [this](auto& req, auto&) { return systemHandlers_.handleInfo(req); }},
        {"GET",  "/api/system/check-internet",   [this](auto& req, auto&) { return systemHandlers_.handleCheckInternet(req); }},
        {"POST", "/api/system/restart",          [this](auto& req, auto&) { return systemHandlers_.handleRestart(req); }},
        {"POST", "/api/system/shutdown",         [this](auto& req, auto&) { return systemHandlers_.handleShutdown(req); }},
        {"GET",  "/api/system/logs",             [this](auto& req, auto& qp) { return systemHandlers_.handleLogs(req, qp); }},

        // Updates (A/B image update via RAUC + tryboot)
        {"GET",  "/api/system/updates/latest",         [this](auto& req, auto&) { return updateHandlers_.handleLatest(req); }},
        {"POST", "/api/system/updates/perform-update", [this](auto& req, auto&) { return updateHandlers_.handlePerformUpdate(req); }},
        {"GET",  "/api/system/updates/status",         [this](auto& req, auto&) { return updateHandlers_.handleStatus(req); }},
        {"GET",  "/api/system/updates/history",        [this](auto& req, auto&) { return updateHandlers_.handleHistory(req); }},

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

    // AP fallback: joining the AP (SSID + password) is treated as implicit
    // auth for the WiFi-setup endpoints only — explicit allowlist, NOT a
    // prefix, so e.g. /api/connection/ap-password stays session-gated.
    static const std::vector<std::string> kApBypassPaths = {
        "/api/connection/status",
        "/api/connection/saved-networks",
        "/api/connection/connect",
        "/api/connection/forget",
    };
    const bool apBypass = _isApMode()
        && std::find(kApBypassPaths.begin(), kApBypassPaths.end(), path) != kApBypassPaths.end();

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
    {
        // Offline fallback: local password instead of the upstream OTP.
        if (auto pwIt = body.find("password"); pwIt != body.end() && pwIt->is_string())
            return _handlePasswordLogin(pwIt->get<std::string>());
        return errorResponse(400, "Bad Request", "Missing otp or password field");
    }

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

    return _sessionLoginResponse();
}

ix::HttpResponsePtr DashboardServer::_sessionLoginResponse() const
{
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

bool DashboardServer::_verifyDashboardPassword(const std::string& password) const
{
    if (auto stored = settings_.get(kPasswordHashKey))
    {
        if (PasswordHash::isWellFormed(*stored))
            return PasswordHash::verify(password, *stored);
        // Treat like an absent row (same trust level as the documented
        // delete-row recovery) so a corrupted value can't brick the login.
        logger_->error("Stored dashboard password hash is malformed, falling back to initial password");
    }
    return !cfg_.secrets.dashboardInitialPassword.empty()
        && PasswordHash::constantTimeEquals(password, cfg_.secrets.dashboardInitialPassword);
}

bool DashboardServer::_isPasswordLoginConfigured() const
{
    if (auto stored = settings_.get(kPasswordHashKey); stored && PasswordHash::isWellFormed(*stored))
        return true;
    return !cfg_.secrets.dashboardInitialPassword.empty();
}

ix::HttpResponsePtr DashboardServer::_handlePasswordLogin(const std::string& password) const
{
    if (int wait = throttle_.retryAfterSecs(); wait > 0)
        return errorResponse(429, "Too Many Requests",
                             "Zu viele Fehlversuche. Bitte warte kurz und versuche es erneut.");

    if (!_isPasswordLoginConfigured())
        return errorResponse(403, "Forbidden", kNotConfiguredMsg);

    if (!_verifyDashboardPassword(password))
    {
        throttle_.recordFailure();
        // Flat-rate failed attempts; each connection has its own thread.
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        return errorResponse(401, "Unauthorized", "Passwort ungültig");
    }

    throttle_.recordSuccess();
    return _sessionLoginResponse();
}

ix::HttpResponsePtr DashboardServer::_handleChangePassword(const ix::HttpRequestPtr& req) const
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

    const std::string currentPassword = body.value("current_password", "");
    const std::string newPassword = body.value("new_password", "");

    if (int wait = throttle_.retryAfterSecs(); wait > 0)
        return errorResponse(429, "Too Many Requests",
                             "Zu viele Fehlversuche. Bitte warte kurz und versuche es erneut.");

    if (!_isPasswordLoginConfigured())
        return errorResponse(403, "Forbidden", kNotConfiguredMsg);

    if (!dashboard::Validation::isValidPassword(newPassword))
        return errorResponse(400, "Bad Request",
                             "Neues Passwort ungültig (8–63 Zeichen, keine Steuerzeichen)");

    if (!_verifyDashboardPassword(currentPassword))
    {
        throttle_.recordFailure();
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        return errorResponse(401, "Unauthorized", "Aktuelles Passwort ungültig");
    }

    throttle_.recordSuccess();
    settings_.set(kPasswordHashKey, PasswordHash::hash(newPassword));
    logger_->info("Dashboard password changed");
    return jsonResponse(200, "OK", nlohmann::json::object(), "Passwort geändert");
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

ix::HttpResponsePtr DashboardServer::_handleConnectionMode(const ix::HttpRequestPtr& req) const
{
    auto mode = wifi_.getWifiMode();
    // AP clients implicitly know the AP password; a logged-in owner may read it
    // (e.g. to note it before it changes). Everyone else on the LAN must not.
    if (mode.value("mode", "") != "ap" && !_hasValidSession(req))
        mode["ap_password"] = "";
    return jsonResponse(200, "OK", mode);
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
    if (!_hasValidSession(req))
    {
        return errorResponse(401, "Unauthorized", "Authentication required");
    }
    return nullptr; // authenticated
}

bool DashboardServer::_hasValidSession(const ix::HttpRequestPtr& req) const
{
    const std::string sessionId = _extractSessionId(req);
    return !sessionId.empty() && sessions_.isValid(sessionId);
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

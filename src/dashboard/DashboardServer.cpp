#include "dashboard/DashboardServer.hpp"
#include "auth/PasswordHash.hpp"
#include "auth/TokenAuth.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "util/HttpUtil.hpp"
#include "util/PanelCode.hpp"
#include "util/Subprocess.hpp"

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
                                 IpcClient& updateIpc, IpcClient& wsIpc, SettingsRepository& settings,
                                 IImageRepository& images)
    : cfg_(cfg), http_(http), sessions_(sessions),
      authTokenManager_(authTokenManager),
      settings_(settings), wifi_(wifi),
      server_(cfg.dashboardApplication.port, cfg.dashboardApplication.host),
      logger_(spdlog::default_logger()->clone("DashboardServer")),
      diagnostics_(cfg, http, authTokenManager),
      wifiHandlers_(wifi, wsIpc, cfg.display.safeModePath),
      frameHandlers_(ipc, cfg),
      slideshowImageHandlers_(ipc, images),
      systemHandlers_(cfg, wifi, diagnostics_, ipc),
      serviceHandlers_(cfg),
      updateHandlers_(updateIpc),
      factoryResetHandlers_(ipc, updateIpc)
{
    _initRoutes();
}

void DashboardServer::start()
{
    server_.setOnConnectionCallback(
        [this](const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>& connState)
        {
            auto res = _handleRequest(req, connState);
            // ix serves one request per connection.
            res->headers["Connection"] = "close";
            return res;
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
        // Public so the SPA can route to AP-setup before any login.
        {"GET",  "/api/connection/mode", [this](auto& req, auto&) { return _handleConnectionMode(req); }},
        // A code on the panel proves presence, so these need no session (and a session alone is not enough).
        {"POST", "/api/system/factory-reset/code",   [this](auto& req, auto&) { return factoryResetHandlers_.handleRequestCode(req); }},
        {"POST", "/api/system/factory-reset",        [this](auto& req, auto&) { return factoryResetHandlers_.handleReset(req); }},
        {"POST", "/api/system/factory-reset/cancel", [this](auto& req, auto&) { return factoryResetHandlers_.handleCancel(req); }},
    };

    protectedRoutes_ = {
        // Auth
        {"POST", "/api/auth/change-password", [this](auto& req, auto&) { return _handleChangePassword(req); }},

        // Frame / Slideshow
        {"GET",  "/api/frame/slideshow/status",   [this](auto& req, auto&) { return frameHandlers_.handleStatus(req); }},
        {"POST", "/api/frame/slideshow",          [this](auto& req, auto&) { return frameHandlers_.handleControl(req); }},
        {"POST", "/api/frame/slideshow/interval", [this](auto& req, auto&) { return frameHandlers_.handleUpdateInterval(req); }},
        {"POST", "/api/frame/slideshow/night-mode", [this](auto& req, auto&) { return frameHandlers_.handleUpdateNightMode(req); }},
        {"POST", "/api/frame/slideshow/skip",     [this](auto& req, auto&) { return frameHandlers_.handleSkip(req); }},
        {"POST", "/api/frame/slideshow/show",     [this](auto& req, auto&) { return frameHandlers_.handleShow(req); }},
        {"POST", "/api/frame/clear",              [this](auto& req, auto&) { return frameHandlers_.handleClear(req); }},
        {"GET",  "/api/frame/display/stats",      [this](auto& req, auto&) { return frameHandlers_.handleDisplayStats(req); }},
        {"GET",  "/api/frame/display/customization", [this](auto& req, auto&) { return frameHandlers_.handleCustomization(req); }},
        {"POST", "/api/frame/display/customization", [this](auto& req, auto&) { return frameHandlers_.handleUpdateCustomization(req); }},
        {"POST", "/api/frame/slideshow/order",    [this](auto& req, auto&) { return slideshowImageHandlers_.handleUpdateOrder(req); }},
        {"GET",  "/api/frame/slideshow/images",             [this](auto& req, auto&) { return slideshowImageHandlers_.handleList(req); }},
        {"GET",  "/api/frame/images/file",        [this](auto& req, auto& qp) { return slideshowImageHandlers_.handleFile(req, qp); }},

        // WiFi / Connection
        {"GET",  "/api/connection/status",         [this](auto& req, auto&) { return wifiHandlers_.handleStatus(req); }},
        {"GET",  "/api/connection/saved-networks", [this](auto& req, auto&) { return wifiHandlers_.handleSavedNetworks(req); }},
        {"POST", "/api/connection/connect",        [this](auto& req, auto&) { return wifiHandlers_.handleConnect(req); }},
        {"POST", "/api/connection/forget",         [this](auto& req, auto&) { return wifiHandlers_.handleForget(req); }},
        {"POST", "/api/connection/apply",          [this](auto& req, auto&) { return wifiHandlers_.handleApply(req); }},
        {"POST", "/api/connection/ap-password",    [this](auto& req, auto&) { return wifiHandlers_.handleSetApPassword(req); }},

        // System
        {"GET",  "/api/system/info",             [this](auto& req, auto&) { return systemHandlers_.handleInfo(req); }},
        {"POST", "/api/system/restart",          [this](auto& req, auto&) { return systemHandlers_.handleRestart(req); }},
        {"POST", "/api/system/shutdown",         [this](auto& req, auto&) { return systemHandlers_.handleShutdown(req); }},
        {"POST", "/api/system/safe-mode/exit",   [this](auto& req, auto&) { return systemHandlers_.handleExitSafeMode(req); }},
        {"GET",  "/api/system/logs",             [this](auto& req, auto& qp) { return systemHandlers_.handleLogs(req, qp); }},
        {"POST", "/api/system/diagnostics",      [this](auto& req, auto&) { return systemHandlers_.handleSendDiagnostics(req); }},

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
    logger_->debug("{} {} from {}:{}", req->method, req->uri,
                   connState->getRemoteIp(), connState->getRemotePort());

    // An exception escaping here would end the whole process.
    try
    {
        return _dispatch(req);
    }
    catch (const nlohmann::json::exception& e)
    {
        logger_->warn("Rejected {} {}: {}", req->method, req->uri, e.what());
        return errorResponse(400, "Bad Request", "Invalid request body");
    }
    catch (const std::exception& e)
    {
        logger_->error("{} {} failed: {}", req->method, req->uri, e.what());
        return errorResponse(500, "Internal Server Error", "Internal error");
    }
}

ix::HttpResponsePtr DashboardServer::_dispatch(const ix::HttpRequestPtr& req) const
{
    auto [path, queryParams] = HttpUtil::parseUri(req->uri);

    if (req->method != "GET" && !_sameOrigin(req))
    {
        logger_->warn("Refused cross-origin {} {} from {}", req->method, path, req->headers.at("origin"));
        return errorResponse(403, "Forbidden", "Cross-origin request refused");
    }

    for (const auto& [method, routePath, handler] : publicRoutes_)
        if (req->method == method && path == routePath)
            return handler(req, queryParams);

    if (auto denied = _loginRequired(req))
        return denied;

    for (const auto& [method, routePath, handler] : protectedRoutes_)
        if (req->method == method && path == routePath)
            return handler(req, queryParams);

    return errorResponse(404, "Not Found", "Not found");
}

// --- Auth Endpoints ---

ix::HttpResponsePtr DashboardServer::_handleLogin(const ix::HttpRequestPtr& req) const
{
    const auto body = nlohmann::json::parse(req->body, nullptr, false);
    if (!body.is_object())
        return errorResponse(400, "Bad Request", "Invalid JSON");

    if (const auto it = body.find("setup_code"); it != body.end() && it->is_string())
        return _handleSetupCodeLogin(req, it->get<std::string>());
    if (const auto it = body.find("otp"); it != body.end() && it->is_string())
        return _handleOtpLogin(req, it->get<std::string>());
    // Offline fallback: local password instead of the upstream OTP.
    if (const auto it = body.find("password"); it != body.end() && it->is_string())
        return _handlePasswordLogin(req, it->get<std::string>());
    return errorResponse(400, "Bad Request", "Missing otp or password field");
}

ix::HttpResponsePtr DashboardServer::_handleOtpLogin(const ix::HttpRequestPtr& req, const std::string& otp) const
{
    const auto attempt = throttle_.begin(_client(req));
    if (!attempt.admitted())
        return _throttled(attempt);

    HTTPClient::Headers headers;
    for (auto& [k, v] : TokenAuth::buildTokenAuthHeaders(authTokenManager_))
        headers[k] = v;
    headers["Content-Type"] = "application/json";

    const std::string url = cfg_.httpBaseUrl() + cfg_.dashboardApplication.httpVerifyOtpUrl;
    const auto resp = otpHttp_.post(url, nlohmann::json{{"otp", otp}}.dump(), headers);
    if (!resp.ok())
    {
        // A wrong code comes back as 401 with "error"; a rejected frame token as 401 with "detail".
        if (const auto refusal = nlohmann::json::parse(resp.body, nullptr, false);
            resp.statusCode == 401 && refusal.is_object() && refusal.contains("error"))
            return errorResponse(401, "Unauthorized", "OTP invalid");
        logger_->warn("OTP verification request failed: {} {}", resp.statusCode, resp.errorMsg);
        return errorResponse(401, "Unauthorized", "OTP verification failed");
    }

    const auto serverResp = nlohmann::json::parse(resp.body, nullptr, false);
    if (!serverResp.is_object() || !serverResp.value("valid", false))
        return errorResponse(401, "Unauthorized", "OTP invalid");

    attempt.succeeded();
    return _sessionLoginResponse(SessionOrigin::OneTimeCode);
}

ix::HttpResponsePtr DashboardServer::_handleSetupCodeLogin(const ix::HttpRequestPtr& req, const std::string& code) const
{
    if (!wifi_.isApMode())
        return errorResponse(409, "Conflict", "Not in access point mode");

    const auto attempt = throttle_.begin(_client(req));
    if (!attempt.admitted())
        return _throttled(attempt);

    const auto expected = panel_code::read(cfg_.display.setupCodePath);
    if (expected.empty() || !PasswordHash::constantTimeEquals(code, expected))
        return errorResponse(401, "Unauthorized", "Setup code invalid");

    attempt.succeeded();
    logger_->info("Signed in with the setup code from the panel");
    Subprocess::run({"shareframe-event", "dashboard", "signed in with the setup code"}, 5);
    return _sessionLoginResponse(SessionOrigin::SetupCode);
}

ix::HttpResponsePtr DashboardServer::_sessionLoginResponse(const SessionOrigin origin) const
{
    const std::string sessionId = sessions_.createSession(origin);
    if (sessionId.empty())
    {
        logger_->error("No randomness for a session id");
        return errorResponse(500, "Internal Server Error", "Internal error");
    }
    logger_->info("Login successful, created session");

    ix::WebSocketHttpHeaders respHeaders;
    respHeaders["Content-Type"] = "application/json";
    respHeaders["Set-Cookie"] = "session=" + sessionId
        + "; HttpOnly; SameSite=Lax; Path=/";

    // Same envelope as jsonResponse, but by hand to attach the Set-Cookie header.
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
        // Treat like an absent row so a corrupted value can't brick the login.
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

ix::HttpResponsePtr DashboardServer::_handlePasswordLogin(const ix::HttpRequestPtr& req,
                                                          const std::string& password) const
{
    if (!_isPasswordLoginConfigured())
        return errorResponse(403, "Forbidden", kNotConfiguredMsg);

    const auto attempt = throttle_.begin(_client(req));
    if (!attempt.admitted())
        return _throttled(attempt);
    if (!_verifyDashboardPassword(password))
        return errorResponse(401, "Unauthorized", "Passwort ungültig");

    attempt.succeeded();
    return _sessionLoginResponse(SessionOrigin::Password);
}

ix::HttpResponsePtr DashboardServer::_handleChangePassword(const ix::HttpRequestPtr& req) const
{
    const auto body = nlohmann::json::parse(req->body, nullptr, false);
    if (!body.is_object())
        return errorResponse(400, "Bad Request", "Invalid JSON");

    const std::string newPassword = body.value("new_password", "");
    if (!dashboard::Validation::isValidPassword(newPassword))
        return errorResponse(400, "Bad Request",
                             "Neues Passwort ungültig (8–63 Zeichen, keine Steuerzeichen)");

    const auto sessionId = _extractSessionId(req);
    if (_needsCurrentPassword(sessionId))
    {
        if (!_isPasswordLoginConfigured())
            return errorResponse(403, "Forbidden", kNotConfiguredMsg);
        const auto attempt = throttle_.begin(_client(req));
        if (!attempt.admitted())
            return _throttled(attempt);
        if (!_verifyDashboardPassword(body.value("current_password", "")))
            return errorResponse(401, "Unauthorized", "Aktuelles Passwort ungültig");
        attempt.succeeded();
    }

    settings_.set(kPasswordHashKey, PasswordHash::hash(newPassword));
    // Whoever knew the old password is signed out everywhere else.
    sessions_.removeAllExcept(sessionId);
    logger_->info("Dashboard password changed");
    return jsonResponse(200, "OK", nlohmann::json::object(), "Passwort geändert");
}

ix::HttpResponsePtr DashboardServer::_handleCheckAuth(const ix::HttpRequestPtr& req) const
{
    const bool authenticated = _hasValidSession(req);
    return jsonResponse(200, "OK", {
        {"authenticated", authenticated},
        {"password_change_needs_current", !authenticated || _needsCurrentPassword(_extractSessionId(req))},
    });
}

ix::HttpResponsePtr DashboardServer::_handleLogout(const ix::HttpRequestPtr& req) const
{
    if (const std::string sessionId = _extractSessionId(req); !sessionId.empty())
        sessions_.removeSession(sessionId);
    return jsonResponse(200, "OK", nlohmann::json::object());
}

ix::HttpResponsePtr DashboardServer::_handleConnectionMode(const ix::HttpRequestPtr& req) const
{
    auto mode = wifiHandlers_.connectionMode();
    // AP clients + a logged-in owner may read the AP password; nobody else on the LAN.
    if (mode.value("mode", "") != "ap" && !_hasValidSession(req))
        mode["ap_password"] = "";
    return jsonResponse(200, "OK", mode);
}

// --- Helpers ---

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
    if (sessionId.empty())
        return false;
    const auto origin = sessions_.origin(sessionId);
    // The setup code only stands for presence while the frame hosts its hotspot.
    if (origin == SessionOrigin::SetupCode && wifi_.getWifiMode().value("mode", "") == "connected")
    {
        sessions_.removeOrigin(SessionOrigin::SetupCode);
        return false;
    }
    return origin.has_value();
}

bool DashboardServer::_needsCurrentPassword(const std::string& sessionId) const
{
    // The server (one-time code) or the panel (setup code) already proved the owner.
    return sessions_.origin(sessionId).value_or(SessionOrigin::Password) == SessionOrigin::Password;
}

bool DashboardServer::_sameOrigin(const ix::HttpRequestPtr& req)
{
    const auto origin = req->headers.find("origin");
    if (origin == req->headers.end())
        return true;
    const auto host = req->headers.find("host");
    return host != req->headers.end() && HttpUtil::hostOf(origin->second) == HttpUtil::hostOf(host->second);
}

std::string DashboardServer::_client(const ix::HttpRequestPtr& req)
{
    // nginx in front sets it; the backend only listens on loopback.
    const auto it = req->headers.find("x-real-ip");
    return it == req->headers.end() ? std::string{} : it->second;
}

ix::HttpResponsePtr DashboardServer::_throttled(const LoginThrottle::Attempt& attempt)
{
    return jsonResponse(429, "Too Many Requests", {{"retry_after_secs", attempt.retryAfterSecs()}},
                        "Zu viele Fehlversuche. Bitte warte kurz und versuche es erneut.");
}

std::string DashboardServer::_extractSessionId(const ix::HttpRequestPtr& req)
{
    const auto it = req->headers.find("cookie");
    if (it == req->headers.end())
        return {};

    // "name=value; name=value": only the cookie named exactly "session" counts.
    std::string_view cookies = it->second;
    while (!cookies.empty())
    {
        const auto end = cookies.find(';');
        auto pair = cookies.substr(0, end);
        cookies = end == std::string_view::npos ? std::string_view{} : cookies.substr(end + 1);
        while (!pair.empty() && pair.front() == ' ')
            pair.remove_prefix(1);
        if (pair.starts_with("session="))
            return std::string(pair.substr(8));
    }
    return {};
}

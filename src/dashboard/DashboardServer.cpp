#include "dashboard/DashboardServer.hpp"
#include "auth/Ed25519Auth.hpp"
#include "auth/ServerSignedResponse.hpp"

DashboardServer::DashboardServer(const AppConfig& cfg, IpcClient& ipc,
                                 HTTPClient& http, SessionManager& sessions)
    : cfg_(cfg), ipc_(ipc), http_(http), sessions_(sessions),
      server_(cfg.dashboardApplication.port, cfg.dashboardApplication.host),
      logger_(spdlog::default_logger()->clone("DashboardServer"))
{
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

ix::HttpResponsePtr DashboardServer::_handleRequest(
    const ix::HttpRequestPtr& req, const std::shared_ptr<ix::ConnectionState>& connState) const
{
    logger_->debug("{} {} from {}:{}", req->method, req->uri,
                   connState->getRemoteIp(), connState->getRemotePort());

    // Auth endpoints
    if (req->uri == "/api/auth/login" && req->method == "POST")
        return _handleLogin(req);
    if (req->uri == "/api/auth/check-auth" && req->method == "GET")
        return _handleCheckAuth(req);
    if (req->uri == "/api/auth/logout" && req->method == "POST")
        return _handleLogout(req);

    // Slideshow endpoints (all require auth)
    if (req->uri == "/api/frame/slideshow/skip-slideshow-image" && req->method == "POST")
    {
        if (auto denied = _loginRequired(req))
            return denied;
        return _handleSkipImage(req);
    }
    if (req->uri == "/api/frame/slideshow/display-images-loop-interval" && req->method == "GET")
    {
        if (auto denied = _loginRequired(req))
            return denied;
        return _handleGetInterval(req);
    }
    if (req->uri == "/api/frame/slideshow/display-images-loop-interval" && req->method == "POST")
    {
        if (auto denied = _loginRequired(req))
            return denied;
        return _handleUpdateInterval(req);
    }

    return _jsonResponse(404, "Not Found", {{"error", "Not found"}});
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
        return _jsonResponse(400, "Bad Request", {{"success", false}, {"message", "Invalid JSON"}});
    }

    auto otpIt = body.find("otp");
    if (otpIt == body.end() || !otpIt->is_string())
        return _jsonResponse(400, "Bad Request", {{"success", false}, {"message", "Missing otp field"}});

    auto otp = otpIt->get<std::string>();

    // Build Ed25519 auth headers for the board→server request
    auto authHeaders = Ed25519Auth::buildHTTPAuthHeaders(cfg_);

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
        return _jsonResponse(401, "Unauthorized",
                             {{"success", false}, {"message", "OTP verification failed"}});
    }

    // Parse server response and extract signed_response
    nlohmann::json serverResp;
    try
    {
        serverResp = nlohmann::json::parse(resp.body);
    }
    catch (...)
    {
        logger_->error("Failed to parse OTP server response");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "Internal error"}});
    }

    auto srIt = serverResp.find("signed_response");
    if (srIt == serverResp.end() || !srIt->is_string())
    {
        logger_->error("Server response missing signed_response field");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "Internal error"}});
    }

    // Verify server's Ed25519 signature
    std::string dataJson;
    try
    {
        dataJson = ServerSignedResponse::verify(srIt->get<std::string>(), cfg_);
    }
    catch (const std::exception& e)
    {
        logger_->error("Signed response verification failed: {}", e.what());
        return _jsonResponse(401, "Unauthorized",
                             {{"success", false}, {"message", "Server verification failed"}});
    }

    // Check that data contains valid=true
    nlohmann::json data;
    try
    {
        data = nlohmann::json::parse(dataJson);
    }
    catch (...)
    {
        logger_->error("Failed to parse verified data payload");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "Internal error"}});
    }

    if (!data.value("valid", false))
    {
        return _jsonResponse(401, "Unauthorized",
                             {{"success", false}, {"message", "OTP invalid"}});
    }

    // Create session
    std::string sessionId = sessions_.createSession();
    logger_->info("Login successful, created session");

    ix::WebSocketHttpHeaders respHeaders;
    respHeaders["Content-Type"] = "application/json";
    respHeaders["Set-Cookie"] = "session=" + sessionId
        + "; HttpOnly; SameSite=Lax; Path=/; Max-Age=604800";

    nlohmann::json respBody = {{"success", true}, {"message", "Login erfolgreich"}};
    return std::make_shared<ix::HttpResponse>(
        200, "OK", ix::HttpErrorCode::Ok, respHeaders, respBody.dump());
}

ix::HttpResponsePtr DashboardServer::_handleCheckAuth(const ix::HttpRequestPtr& req) const
{
    const std::string sessionId = _extractSessionId(req);
    bool authenticated = !sessionId.empty() && sessions_.isValid(sessionId);
    return _jsonResponse(200, "OK", {{"authenticated", authenticated}});
}

ix::HttpResponsePtr DashboardServer::_handleLogout(const ix::HttpRequestPtr& req) const
{
    if (const std::string sessionId = _extractSessionId(req); !sessionId.empty())
        sessions_.removeSession(sessionId);
    return _jsonResponse(200, "OK", {{"success", true}});
}

// --- Slideshow Endpoints ---

ix::HttpResponsePtr DashboardServer::_handleSkipImage(const ix::HttpRequestPtr& /*req*/) const
{
    if (const IpcMessage msg{IpcMessageType::SkipImage, {}}; !ipc_.send(msg))
    {
        logger_->error("Failed to send skip_image via IPC");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "IPC error"}});
    }
    return _jsonResponse(200, "OK", {{"success", true}});
}

ix::HttpResponsePtr DashboardServer::_handleGetInterval(const ix::HttpRequestPtr& /*req*/) const
{
    const auto result = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetDisplayInterval, {}});
    if (!result)
    {
        logger_->error("Failed to query display interval via IPC");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "Service unavailable"}});
    }

    int intervalSecs = result->value("interval_secs", cfg_.display.intervalSecs);
    return _jsonResponse(200, "OK", {{"success", true}, {"interval_seconds", intervalSecs}});
}

ix::HttpResponsePtr DashboardServer::_handleUpdateInterval(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try
    {
        body = nlohmann::json::parse(req->body);
    }
    catch (...)
    {
        return _jsonResponse(400, "Bad Request", {{"success", false}, {"message", "Invalid JSON"}});
    }

    int secs = body.value("interval_seconds", 0);
    if (secs < 180 || secs > 86400)
    {
        return _jsonResponse(400, "Bad Request",
                             {{"success", false}, {"message", "interval_seconds must be between 180 and 86400"}});
    }

    const nlohmann::json data = {{"interval_secs", secs}};
    if (const IpcMessage msg{IpcMessageType::UpdateDisplayInterval, data}; !ipc_.send(msg))
    {
        logger_->error("Failed to send update_display_interval via IPC");
        return _jsonResponse(500, "Internal Server Error",
                             {{"success", false}, {"message", "IPC error"}});
    }

    return _jsonResponse(200, "OK", {{"success", true}, {"interval_seconds", secs}});
}

// --- Helpers ---

ix::HttpResponsePtr DashboardServer::_jsonResponse(int status, const std::string& statusText,
                                                   const nlohmann::json& body)
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    return std::make_shared<ix::HttpResponse>(
        status, statusText, ix::HttpErrorCode::Ok, headers, body.dump());
}

ix::HttpResponsePtr DashboardServer::_loginRequired(const ix::HttpRequestPtr& req) const
{
    if (const std::string sessionId = _extractSessionId(req); sessionId.empty() || !sessions_.isValid(sessionId))
    {
        return _jsonResponse(401, "Unauthorized",
                             {{"success", false}, {"message", "Authentication required"}});
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

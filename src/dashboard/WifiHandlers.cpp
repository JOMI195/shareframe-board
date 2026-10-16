#include "dashboard/WifiHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "net/ServerConnection.hpp"
#include <algorithm>
#include <ctime>

using namespace dashboard;

WifiHandlers::WifiHandlers(WifiManager& wifi, IpcClient& wsIpc)
    : wifi_(wifi), wsIpc_(wsIpc), logger_(spdlog::default_logger()->clone("WifiHandlers"))
{
}

static ix::HttpResponsePtr wifiResult(const nlohmann::json& result)
{
    if (result.contains("error"))
        return errorResponse(500, "Internal Server Error", result["error"].get<std::string>());
    return jsonResponse(200, "OK", result);
}

ix::HttpResponsePtr WifiHandlers::handleStatus(const ix::HttpRequestPtr& /*req*/) const
{
    return wifiResult(wifi_.getCurrentConnection());
}

nlohmann::json WifiHandlers::connectionMode() const
{
    auto mode = wifi_.getWifiMode();
    const auto server = queryServerConnection(wsIpc_);
    mode["server_connected"] = server ? nlohmann::json(server->connected) : nlohmann::json();
    mode["server_offline_secs"] = server && !server->connected && server->since > 0
        ? std::max<int64_t>(0, std::time(nullptr) - server->since) : 0;
    mode["server_last_contact_at"] = server ? server->lastContactAt : 0;
    return mode;
}

ix::HttpResponsePtr WifiHandlers::handleSavedNetworks(const ix::HttpRequestPtr& /*req*/) const
{
    return wifiResult(wifi_.getSavedNetworks());
}

ix::HttpResponsePtr WifiHandlers::handleConnect(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    auto ssid = body.value("ssid", "");
    auto password = body.value("password", "");

    if (!Validation::isValidSsid(ssid))
        return errorResponse(400, "Bad Request", "Invalid SSID (max 32 bytes, no control chars)");

    if (!Validation::isValidPassword(password))
        return errorResponse(400, "Bad Request", "Invalid password (8-63 chars, no control chars)");

    return wifiResult(wifi_.connect(ssid, password));
}

ix::HttpResponsePtr WifiHandlers::handleForget(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    auto ssid = body.value("ssid", "");
    if (!Validation::isValidNetworkName(ssid))
        return errorResponse(400, "Bad Request", "Invalid network name");

    return wifiResult(wifi_.forget(ssid));
}

ix::HttpResponsePtr WifiHandlers::handleSetApPassword(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    auto password = body.value("password", "");
    if (!Validation::isValidWpaPassphrase(password))
        return errorResponse(400, "Bad Request", "Ungültiges AP-Passwort (8–63 ASCII-Zeichen)");

    return wifiResult(wifi_.setApPassword(password));
}

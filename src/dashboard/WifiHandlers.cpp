#include "dashboard/WifiHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"

using namespace dashboard;

WifiHandlers::WifiHandlers(WifiManager& wifi)
    : wifi_(wifi), logger_(spdlog::default_logger()->clone("WifiHandlers"))
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

ix::HttpResponsePtr WifiHandlers::handleMode(const ix::HttpRequestPtr& /*req*/) const
{
    // Public: the SPA reads this before any login to decide between the normal
    // dashboard and the offline AP-setup view.
    return jsonResponse(200, "OK", wifi_.getWifiMode());
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

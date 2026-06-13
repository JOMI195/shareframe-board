#include "dashboard/UpdateHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include <nlohmann/json.hpp>

using dashboard::errorResponse;
using dashboard::jsonResponse;
using namespace std::chrono_literals;

UpdateHandlers::UpdateHandlers(IpcClient& updateIpc)
    : updateIpc_(updateIpc), logger_(spdlog::default_logger()->clone("UpdateHandlers"))
{
}

ix::HttpResponsePtr UpdateHandlers::handleLatest(const ix::HttpRequestPtr& /*req*/) const
{
    // server round-trip happens inside the update service -> generous timeout
    auto resp = updateIpc_.sendAndReceive({IpcMessageType::UpdateCheckLatest, {}}, 30000ms);
    if (!resp)
        return errorResponse(503, "Service Unavailable", "Update service not reachable");
    if (!resp->value("ok", false))
    {
        if (resp->value("status_code", 0) == 404)
            return jsonResponse(200, "OK", nullptr);  // no release available
        logger_->error("Update check failed: {}", resp->value("error", ""));
        return errorResponse(502, "Bad Gateway", "Failed to check for updates");
    }
    return jsonResponse(200, "OK", (*resp)["release"]);
}

ix::HttpResponsePtr UpdateHandlers::handlePerformUpdate(const ix::HttpRequestPtr& /*req*/) const
{
    // startUpdate validates against the server before spawning the worker
    auto resp = updateIpc_.sendAndReceive({IpcMessageType::UpdatePerform, {}}, 30000ms);
    if (!resp)
        return errorResponse(503, "Service Unavailable", "Update service not reachable");
    if (!resp->value("ok", false))
    {
        const auto error = resp->value("error", "Update not started");
        logger_->warn("Update not started: {}", error);
        return errorResponse(409, "Conflict", error);
    }
    logger_->info("Update started via dashboard");
    return jsonResponse(200, "OK", nlohmann::json::object(), "Update started");
}

ix::HttpResponsePtr UpdateHandlers::handleStatus(const ix::HttpRequestPtr& /*req*/) const
{
    auto resp = updateIpc_.sendAndReceive({IpcMessageType::UpdateStatus, {}});
    if (!resp || !resp->value("ok", false))
        return errorResponse(503, "Service Unavailable", "Update service not reachable");
    return jsonResponse(200, "OK", (*resp)["status"]);
}

ix::HttpResponsePtr UpdateHandlers::handleHistory(const ix::HttpRequestPtr& /*req*/) const
{
    auto resp = updateIpc_.sendAndReceive({IpcMessageType::UpdateHistory, {}});
    if (!resp || !resp->value("ok", false))
        return errorResponse(503, "Service Unavailable", "Update service not reachable");
    return jsonResponse(200, "OK", {{"history", (*resp)["history"]}});
}

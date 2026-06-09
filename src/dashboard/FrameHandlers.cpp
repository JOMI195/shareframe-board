#include "dashboard/FrameHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "ipc/IpcProtocol.hpp"

using namespace dashboard;

FrameHandlers::FrameHandlers(IpcClient& ipc, AppConfig& cfg)
    : ipc_(ipc), cfg_(cfg),
      logger_(spdlog::default_logger()->clone("FrameHandlers"))
{
}

ix::HttpResponsePtr FrameHandlers::handleStatus(const ix::HttpRequestPtr& /*req*/) const
{
    auto activeResult = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetSlideshowActive, {}});
    auto intervalResult = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetDisplayInterval, {}});

    if (!activeResult || !intervalResult)
    {
        logger_->error("Failed to query slideshow status via IPC");
        return errorResponse(500, "Internal Server Error", "Service unavailable");
    }

    // Remaining time until the next image change (-1 when paused/unknown).
    // Best-effort: don't fail the whole status if this single call misses.
    auto secondsResult = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetSecondsUntilNext, {}});

    return jsonResponse(200, "OK", {
        {"active", activeResult->value("active", true)},
        {"interval_seconds", intervalResult->value("interval_secs", cfg_.display.intervalSecs)},
        {"seconds_until_next", secondsResult ? secondsResult->value("seconds_until_next", -1) : -1}
    });
}

ix::HttpResponsePtr FrameHandlers::handleControl(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    auto action = body.value("action", "");
    if (!Validation::isValidSlideshowAction(action))
        return errorResponse(400, "Bad Request", "action must be 'start' or 'stop'");

    bool active = (action == "start");
    if (!ipc_.send(IpcMessage{IpcMessageType::SetSlideshowActive, {{"active", active}}}))
    {
        logger_->error("Failed to send set_slideshow_active via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }

    return jsonResponse(200, "OK", {{"action", action}});
}

ix::HttpResponsePtr FrameHandlers::handleUpdateInterval(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    int secs = body.value("interval_seconds", 0);
    if (secs < 180 || secs > 86400)
        return errorResponse(400, "Bad Request", "interval_seconds must be between 180 and 86400");

    if (!ipc_.send(IpcMessage{IpcMessageType::UpdateDisplayInterval, {{"interval_secs", secs}}}))
    {
        logger_->error("Failed to send update_display_interval via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }

    return jsonResponse(200, "OK", {{"interval_seconds", secs}});
}

ix::HttpResponsePtr FrameHandlers::handleSkip(const ix::HttpRequestPtr& /*req*/) const
{
    if (!ipc_.send(IpcMessage{IpcMessageType::SkipImage, {}}))
    {
        logger_->error("Failed to send skip_image via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }
    return jsonResponse(200, "OK", nlohmann::json::object());
}

ix::HttpResponsePtr FrameHandlers::handleClear(const ix::HttpRequestPtr& /*req*/) const
{
    if (!ipc_.send(IpcMessage{IpcMessageType::ClearDisplay, {}}))
    {
        logger_->error("Failed to send clear_display via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }
    return jsonResponse(200, "OK", nlohmann::json::object());
}

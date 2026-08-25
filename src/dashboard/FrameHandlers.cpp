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
    auto nightResult = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetNightMode, {}});

    nlohmann::json nightMode = {
        {"enabled", nightResult ? nightResult->value("enabled", false) : cfg_.display.nightModeEnabled},
        {"start_hour", nightResult ? nightResult->value("start_hour", cfg_.display.nightStartHour) : cfg_.display.nightStartHour},
        {"end_hour", nightResult ? nightResult->value("end_hour", cfg_.display.nightEndHour) : cfg_.display.nightEndHour},
        {"interval_seconds", nightResult ? nightResult->value("interval_secs", cfg_.display.nightIntervalSecs) : cfg_.display.nightIntervalSecs},
        {"active_now", nightResult && nightResult->value("active_now", false)}
    };

    return jsonResponse(200, "OK", {
        {"active", activeResult->value("active", true)},
        {"loop_started", activeResult->value("loop_started", false)},
        {"image_count", activeResult->value("image_count", 0)},
        {"interval_seconds", intervalResult->value("interval_secs", cfg_.display.intervalSecs)},
        {"seconds_until_next", secondsResult ? secondsResult->value("seconds_until_next", -1) : -1},
        {"night_mode", nightMode}
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
    if (!Validation::isValidIntervalSecs(secs))
        return errorResponse(400, "Bad Request", "interval_seconds must be between 180 and 86400");

    if (!ipc_.send(IpcMessage{IpcMessageType::UpdateDisplayInterval, {{"interval_secs", secs}}}))
    {
        logger_->error("Failed to send update_display_interval via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }

    return jsonResponse(200, "OK", {{"interval_seconds", secs}});
}

ix::HttpResponsePtr FrameHandlers::handleUpdateNightMode(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    const bool enabled = body.value("enabled", false);
    const int startHour = body.value("start_hour", -1);
    const int endHour = body.value("end_hour", -1);
    const int secs = body.value("interval_seconds", 0);

    if (!Validation::isValidHour(startHour) || !Validation::isValidHour(endHour))
        return errorResponse(400, "Bad Request", "start_hour and end_hour must be between 0 and 23");
    if (startHour == endHour)
        return errorResponse(400, "Bad Request", "start_hour and end_hour must differ");
    if (!Validation::isValidIntervalSecs(secs))
        return errorResponse(400, "Bad Request", "interval_seconds must be between 180 and 86400");

    if (!ipc_.send(IpcMessage{IpcMessageType::UpdateNightMode, {
            {"enabled", enabled},
            {"start_hour", startHour},
            {"end_hour", endHour},
            {"interval_secs", secs}
        }}))
    {
        logger_->error("Failed to send update_night_mode via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }

    return jsonResponse(200, "OK", {
        {"enabled", enabled},
        {"start_hour", startHour},
        {"end_hour", endHour},
        {"interval_seconds", secs}
    });
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

ix::HttpResponsePtr FrameHandlers::handleDisplayStats(const ix::HttpRequestPtr& /*req*/) const
{
    auto result = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetDisplayStats, {}});
    if (!result)
    {
        logger_->error("Failed to query display stats via IPC");
        return errorResponse(500, "Internal Server Error", "Service unavailable");
    }
    return jsonResponse(200, "OK", *result);
}

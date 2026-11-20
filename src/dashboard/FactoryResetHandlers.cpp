#include "dashboard/FactoryResetHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "ipc/IpcProtocol.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "update/UpdatePhase.hpp"
#include "util/Subprocess.hpp"

using namespace dashboard;
using Issue = FactoryResetGuard::IssueResult;
using Check = FactoryResetGuard::CheckResult;

FactoryResetHandlers::FactoryResetHandlers(IpcClient& displayIpc, IpcClient& updateIpc)
    : displayIpc_(displayIpc), updateIpc_(updateIpc), logger_(spdlog::default_logger()->clone("FactoryReset"))
{
}

ix::HttpResponsePtr FactoryResetHandlers::handleRequestCode(const ix::HttpRequestPtr& /*req*/)
{
    if (const auto reason = _refusal())
        return errorResponse(409, "Conflict", *reason);

    const auto issued = guard_.issue();
    if (issued.result == Issue::Locked)
        return errorResponse(429, "Too Many Requests", "Too many reset codes");
    if (issued.result == Issue::Pending)
        return jsonResponse(200, "OK", {{"expires_in_secs", issued.secondsLeft}, {"shows_in_secs", 0}});

    const auto shown = displayIpc_.sendAndReceive({IpcMessageType::ShowStatusScreen,
                                                   {{"screen", toString(StatusScreenKind::ResetCode)},
                                                    {"code", issued.code},
                                                    {"timeout_secs", issued.secondsLeft}}});
    if (!shown)
    {
        (void)guard_.cancel();
        logger_->warn("Reset code requested, but the display is not reachable");
        return errorResponse(503, "Service Unavailable", "Screen unavailable");
    }
    logger_->info("Reset code shown on the panel");
    _event("reset code requested");
    return jsonResponse(200, "OK", {{"expires_in_secs", issued.secondsLeft},
                                    {"shows_in_secs", shown->value("shows_in_secs", 0)}});
}

ix::HttpResponsePtr FactoryResetHandlers::handleReset(const ix::HttpRequestPtr& req)
{
    const auto body = nlohmann::json::parse(req->body, nullptr, false);
    const auto code = body.is_object() ? body.find("code") : body.end();
    if (!body.is_object() || code == body.end() || !code->is_string())
        return errorResponse(400, "Bad Request", "code is required");
    if (const auto reason = _refusal())
        return errorResponse(409, "Conflict", *reason);

    const auto check = guard_.verify(code->get<std::string>());
    switch (check.result)
    {
    case Check::Locked:
        return errorResponse(429, "Too Many Requests", "Too many reset codes");
    case Check::NoCode:
        return errorResponse(409, "Conflict", "No reset code requested");
    case Check::Wrong:
        _event("wrong reset code");
        if (check.triesLeft == 0)
            displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
        return jsonResponse(403, "Forbidden", {{"tries_left", check.triesLeft}}, "Wrong code");
    case Check::Accepted:
        break;
    }

    logger_->info("Factory reset confirmed with the panel code");
    _event("factory reset with the panel code");
    displayIpc_.send({IpcMessageType::ShowStatusScreen, {{"screen", toString(StatusScreenKind::FactoryReset)}}});
    if (const auto result = Subprocess::run({"shareframe-factory-reset", "--yes"}, 5); result.exitCode != 0)
    {
        logger_->error("shareframe-factory-reset failed: {}", result.stdErr);
        displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
        return errorResponse(500, "Internal Server Error", "Factory reset failed");
    }
    return jsonResponse(200, "OK", nlohmann::json::object(), "Resetting to factory defaults...");
}

ix::HttpResponsePtr FactoryResetHandlers::handleCancel(const ix::HttpRequestPtr& /*req*/)
{
    if (guard_.cancel())
    {
        _event("reset code cancelled");
        displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
    }
    return jsonResponse(200, "OK", nlohmann::json::object());
}

std::optional<std::string> FactoryResetHandlers::_refusal() const
{
    if (const auto status = updateIpc_.sendAndReceive({IpcMessageType::UpdateStatus, {}}))
    {
        const auto phase = updatePhaseFromString((*status)["status"].value("phase", std::string{"idle"}));
        if (!phase || isBusy(*phase))
            return "An update is running";
    }
    if (const auto gate = Subprocess::run({"shareframe-remote-gate", "check", "--no-rate-limit"}, 5); gate.exitCode != 0)
        return "The new version is not confirmed yet";
    return std::nullopt;
}

void FactoryResetHandlers::_event(const std::string& text)
{
    Subprocess::run({"shareframe-event", "reset", text}, 5);
}

#include "dashboard/ServiceHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "ipc/HealthCheck.hpp"
#include "util/Subprocess.hpp"
#include <array>
#include <cctype>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using namespace dashboard;

namespace
{
struct ServiceDesc { const char* id; const char* label; };

// The four split services. ids double as the s6 service suffix
// ("shareframe-<id>") and the /api/system/logs service_name.
constexpr std::array<ServiceDesc, 4> kServices = {{
    {"display",   "Display"},
    {"websocket", "WebSocket"},
    {"dashboard", "Dashboard"},
    {"heartbeat", "Heartbeat"},
}};

bool isAllowedId(const std::string& id)
{
    for (const auto& s : kServices)
        if (id == s.id) return true;
    return false;
}

// Parse `s6-svstat` output, e.g. "up (pid 1234) 567 seconds" or
// "down (exitcode 0) 12 seconds". On a host without s6 (local dev) the command
// fails and we report status "unknown" rather than erroring the whole list.
nlohmann::json parseSvstat(const SubprocessResult& r)
{
    nlohmann::json out = {{"status", "unknown"}, {"uptime_seconds", nullptr}, {"pid", nullptr}};
    if (r.exitCode != 0)
        return out;

    std::istringstream ss(r.stdOut);
    std::string first;
    ss >> first;
    if (first == "up" || first == "down")
        out["status"] = first;

    if (auto p = r.stdOut.find("(pid "); p != std::string::npos)
        try { out["pid"] = std::stoll(r.stdOut.substr(p + 5)); } catch (...) {}

    // integer immediately preceding " seconds" is the time-in-state
    if (auto p = r.stdOut.find(" seconds"); p != std::string::npos)
    {
        size_t b = p;
        while (b > 0 && std::isdigit(static_cast<unsigned char>(r.stdOut[b - 1]))) --b;
        if (b < p)
            try { out["uptime_seconds"] = std::stoll(r.stdOut.substr(b, p - b)); } catch (...) {}
    }
    return out;
}
} // namespace

ServiceHandlers::ServiceHandlers(const AppConfig& cfg)
    : cfg_(cfg),
      displayIpc_(cfg.ipc.displayRep),
      wsIpc_(cfg.ipc.wsRep),
      heartbeatIpc_(cfg.ipc.heartbeatRep),
      logger_(spdlog::default_logger()->clone("ServiceHandlers"))
{
}

ix::HttpResponsePtr ServiceHandlers::handleList(const ix::HttpRequestPtr& /*req*/) const
{
    nlohmann::json services = nlohmann::json::array();
    for (const auto& s : kServices)
    {
        const std::string id = s.id;

        bool running;
        if (id == "dashboard")        running = true; // serving this request
        else if (id == "display")     running = health::isRunning(displayIpc_);
        else if (id == "websocket")   running = health::isRunning(wsIpc_);
        else                          running = health::isRunning(heartbeatIpc_);

        const auto svstat = parseSvstat(
            Subprocess::run({"s6-svstat", "/run/service/shareframe-" + id}, 5));

        services.push_back({
            {"id", id},
            {"label", s.label},
            {"running", running},
            {"status", svstat["status"]},
            {"uptime_seconds", svstat["uptime_seconds"]},
            {"pid", svstat["pid"]},
        });
    }
    return jsonResponse(200, "OK", {{"services", services}});
}

ix::HttpResponsePtr ServiceHandlers::handleRestart(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    const std::string id = body.value("service", "");
    if (!isAllowedId(id))
        return errorResponse(400, "Bad Request", "Invalid or unknown service");

    // s6-svc -r restarts (down then up) the supervised service. Restarting
    // "dashboard" cycles this very process: the reply is sent first, then the
    // supervisor brings us back (nginx serves its 502 page during the gap).
    logger_->info("Restart requested for service {}", id);
    const auto r = Subprocess::run({"s6-svc", "-r", "/run/service/shareframe-" + id}, 5);
    if (r.exitCode != 0)
    {
        logger_->error("s6-svc -r shareframe-{} failed: {}", id, r.stdErr);
        return errorResponse(500, "Internal Server Error", "Failed to restart service");
    }

    return jsonResponse(200, "OK", {{"service", id}}, "Service wird neu gestartet");
}

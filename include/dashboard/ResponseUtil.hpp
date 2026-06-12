#pragma once
#include <ixwebsocket/IXHttpServer.h>
#include <nlohmann/json.hpp>

namespace dashboard {

// Every dashboard response shares one envelope { success, message, data } so the
// frontend reads it uniformly; success is derived from the HTTP status.
inline ix::HttpResponsePtr jsonResponse(int status, const std::string& statusText,
                                        const nlohmann::json& data = nlohmann::json::object(),
                                        const std::string& message = "")
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    const nlohmann::json body = {
        {"success", status >= 200 && status < 300},
        {"message", message},
        {"data", data},
    };
    return std::make_shared<ix::HttpResponse>(
        status, statusText, ix::HttpErrorCode::Ok, headers, body.dump());
}

inline ix::HttpResponsePtr errorResponse(int status, const std::string& statusText,
                                         const std::string& message)
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    const nlohmann::json body = {
        {"success", false},
        {"message", message},
        {"data", nullptr},
    };
    return std::make_shared<ix::HttpResponse>(
        status, statusText, ix::HttpErrorCode::Ok, headers, body.dump());
}

} // namespace dashboard

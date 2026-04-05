#pragma once
#include <ixwebsocket/IXHttpServer.h>
#include <nlohmann/json.hpp>

namespace dashboard {

inline ix::HttpResponsePtr jsonResponse(int status, const std::string& statusText,
                                        const nlohmann::json& body)
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    return std::make_shared<ix::HttpResponse>(
        status, statusText, ix::HttpErrorCode::Ok, headers, body.dump());
}

inline ix::HttpResponsePtr errorResponse(int status, const std::string& statusText,
                                         const std::string& message)
{
    return jsonResponse(status, statusText, {{"error", message}});
}

} // namespace dashboard

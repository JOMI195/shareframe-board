#pragma once
#include <ixwebsocket/IXHttpServer.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

namespace dashboard {

// Every dashboard response shares one envelope { success, message, data } so the
// frontend reads it uniformly; success is derived from the HTTP status.
inline ix::HttpResponsePtr jsonResponse(int status, const std::string& statusText,
                                        const nlohmann::json& data = nlohmann::json::object(),
                                        const std::string& message = "")
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    headers["Cache-Control"] = "no-store";
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
    headers["Cache-Control"] = "no-store";
    const nlohmann::json body = {
        {"success", false},
        {"message", message},
        {"data", nullptr},
    };
    return std::make_shared<ix::HttpResponse>(
        status, statusText, ix::HttpErrorCode::Ok, headers, body.dump());
}

inline ix::HttpResponsePtr binaryResponse(const std::string& contentType, std::string body,
                                          const std::string& cacheControl)
{
    ix::WebSocketHttpHeaders headers;
    headers["Content-Type"] = contentType;
    headers["Cache-Control"] = cacheControl;
    return std::make_shared<ix::HttpResponse>(
        200, "OK", ix::HttpErrorCode::Ok, headers, std::move(body));
}

inline std::optional<std::string> readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::nullopt;
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

/// A stored JPEG or PNG; the type comes from the bytes, not the file name.
inline ix::HttpResponsePtr pictureResponse(std::string bytes, const std::string& cacheControl)
{
    const auto type = bytes.starts_with("\x89PNG") ? "image/png" : "image/jpeg";
    return binaryResponse(type, std::move(bytes), cacheControl);
}

} // namespace dashboard

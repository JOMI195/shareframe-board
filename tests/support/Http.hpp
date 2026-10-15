#pragma once
#include <ixwebsocket/IXHttpServer.h>
#include <nlohmann/json.hpp>

inline ix::HttpRequestPtr httpRequest(const std::string& method, const std::string& uri,
                                      const nlohmann::json& body = nullptr)
{
    return std::make_shared<ix::HttpRequest>(uri, method, "HTTP/1.1", body.is_null() ? "" : body.dump());
}

/// The dashboard's { success, message, data } envelope plus the status code.
struct HttpResult
{
    int status;
    nlohmann::json body;
};

inline HttpResult result(const ix::HttpResponsePtr& resp)
{
    return {resp->statusCode, nlohmann::json::parse(resp->body)};
}

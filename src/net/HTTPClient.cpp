#include "net/HTTPClient.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <spdlog/spdlog.h>

static ix::HttpRequestArgsPtr makeArgs(
    const int connectTimeout,
    const int transferTimeout,
    const HTTPClient::Headers& headers)
{
    auto args = std::make_shared<ix::HttpRequestArgs>();
    args->connectTimeout = connectTimeout;
    args->transferTimeout = transferTimeout;
    for (const auto& [k, v] : headers)
        args->extraHeaders[k] = v;
    return args;
}

static HttpResponse toResponse(const ix::HttpResponsePtr& r)
{
    if (!r)
    {
        spdlog::error("HTTP request failed: null response");
        return {0, "", "null response"};
    }
    if (r->statusCode == 0 && !r->errorMsg.empty())
        spdlog::error("HTTP request failed: {}", r->errorMsg);
    return {r->statusCode, r->body, r->errorMsg};
}

HTTPClient::HTTPClient(const int connectTimeoutSecs, const int transferTimeoutSecs)
    : _connectTimeout(connectTimeoutSecs)
      , _transferTimeout(transferTimeoutSecs)
{
}

HttpResponse HTTPClient::get(const std::string& url, const Headers& headers) const
{
    spdlog::debug("HTTP GET {}", url);
    ix::HttpClient client;
    const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
    return toResponse(client.get(url, args));
}

HttpResponse HTTPClient::post(const std::string& url, const std::string& body, const Headers& headers) const
{
    spdlog::debug("HTTP POST {}", url);
    ix::HttpClient client;
    const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
    return toResponse(client.post(url, body, args));
}

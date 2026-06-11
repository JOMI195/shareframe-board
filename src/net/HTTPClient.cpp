#include "net/HTTPClient.hpp"
#include <cstdio>
#include <fstream>
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

HttpResponse HTTPClient::downloadToFile(const std::string& url, const std::string& destPath,
                                        const Headers& headers, const ProgressFn& progress) const
{
    spdlog::debug("HTTP GET (to file) {} -> {}", url, destPath);

    std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
    if (!out)
        return {0, "", "cannot open " + destPath + " for writing"};

    ix::HttpClient client;
    const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
    bool writeError = false;
    args->onChunkCallback = [&out, &writeError](const std::string& chunk)
    {
        out.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        if (!out)
            writeError = true;
    };
    if (progress)
    {
        args->onProgressCallback = [&progress](int current, int total) -> bool
        {
            progress(current > 0 ? static_cast<size_t>(current) : 0,
                     total > 0 ? static_cast<size_t>(total) : 0);
            return true;
        };
    }

    auto resp = toResponse(client.get(url, args));
    out.close();

    if (writeError && resp.ok())
    {
        resp.statusCode = 0;
        resp.errorMsg = "write failed for " + destPath + " (disk full?)";
    }
    if (!resp.ok())
        std::remove(destPath.c_str());
    return resp;
}

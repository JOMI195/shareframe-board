#include "net/HTTPClient.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <ixwebsocket/IXHttpClient.h>
#include <spdlog/spdlog.h>
#include <thread>

// WebSocket-style backoff: double the wait between a min and max bound on each
// retry, only when the server can't be reached.
constexpr int kMinWaitMs = 1000;
constexpr int kMaxWaitMs = 180000;
constexpr int kMaxRetries = 3;

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
    // Per-attempt transport failures log at debug; the retry wrapper warns once
    // per retry and callers log the final failure.
    if (!r)
    {
        spdlog::debug("HTTP request failed: null response");
        return {0, "", "null response"};
    }
    if (r->statusCode == 0 && !r->errorMsg.empty())
        spdlog::debug("HTTP request failed: {}", r->errorMsg);
    return {r->statusCode, r->body, r->errorMsg};
}

template <typename Fn>
HttpResponse HTTPClient::_withRetry(Fn&& doRequest) const
{
    HttpResponse resp;
    for (int attempt = 0;; ++attempt)
    {
        resp = doRequest();
        if (resp.statusCode != 0 || attempt >= kMaxRetries)
            return resp;
        const int delayMs = std::min(kMinWaitMs << attempt, kMaxWaitMs);
        spdlog::warn("HTTP could not reach server (attempt {}/{}), retrying in {}ms",
                     attempt + 1, kMaxRetries, delayMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

HTTPClient::HTTPClient(const int connectTimeoutSecs, const int transferTimeoutSecs)
    : _connectTimeout(connectTimeoutSecs)
      , _transferTimeout(transferTimeoutSecs)
{
}

HttpResponse HTTPClient::get(const std::string& url, const Headers& headers) const
{
    spdlog::debug("HTTP GET {}", url);
    return _withRetry([&]
    {
        ix::HttpClient client;
        const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
        return toResponse(client.get(url, args));
    });
}

HttpResponse HTTPClient::post(const std::string& url, const std::string& body, const Headers& headers) const
{
    spdlog::debug("HTTP POST {}", url);
    return _withRetry([&]
    {
        ix::HttpClient client;
        const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
        return toResponse(client.post(url, body, args));
    });
}

HttpResponse HTTPClient::downloadToFile(const std::string& url, const std::string& destPath,
                                        const Headers& headers, const ProgressFn& progress) const
{
    spdlog::debug("HTTP GET (to file) {} -> {}", url, destPath);

    HttpResponse resp;
    for (int attempt = 0;; ++attempt)
    {
        std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            resp = {0, "", "cannot open " + destPath + " for writing"};
            break;
        }

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

        resp = toResponse(client.get(url, args));
        out.close();
        if (!out)
            writeError = true;

        if (writeError && resp.ok())
        {
            resp.statusCode = 0;
            resp.errorMsg = "write failed for " + destPath + " (disk full?)";
        }

        // Retry only when the server couldn't be reached; a write error is local.
        if (resp.statusCode != 0 || writeError || attempt >= kMaxRetries)
            break;
        const int delayMs = std::min(kMinWaitMs << attempt, kMaxWaitMs);
        spdlog::warn("HTTP download could not reach server (attempt {}/{}), retrying in {}ms",
                     attempt + 1, kMaxRetries, delayMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    if (!resp.ok())
        std::remove(destPath.c_str());
    return resp;
}

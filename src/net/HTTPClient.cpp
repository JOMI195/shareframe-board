#include "net/HTTPClient.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ixwebsocket/IXHttpClient.h>
#include <spdlog/spdlog.h>
#include <thread>

// WebSocket-style backoff: double the wait between a min and max bound on each
// retry, only when the server can't be reached.
constexpr int kMinWaitMs = 1000;
constexpr int kMaxWaitMs = 180000;
constexpr int kMaxRetries = 3;
// Bounds the requests of one download so a crawling link stays inside the server's quota.
constexpr int kMaxDownloadAttempts = 30;

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
    // A body cut off after the status line is a lost connection, not an answer.
    const bool bodyLost = r->errorCode == ix::HttpErrorCode::ChunkReadError
        || r->errorCode == ix::HttpErrorCode::CannotReadBody;
    const int status = bodyLost ? 0 : r->statusCode;
    if (status == 0 && !r->errorMsg.empty())
        spdlog::debug("HTTP request failed: {}", r->errorMsg);
    return {status, r->body, r->errorMsg};
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
    int failures = 0;
    for (int attempt = 1;; ++attempt)
    {
        std::error_code ec;
        const auto existing = std::filesystem::file_size(destPath, ec);
        const size_t offset = ec ? 0 : static_cast<size_t>(existing);

        std::ofstream out(destPath, std::ios::binary | (offset > 0 ? std::ios::app : std::ios::trunc));
        if (!out)
            return {0, "", "cannot open " + destPath + " for writing"};

        ix::HttpClient client;
        const auto args = makeArgs(_connectTimeout, _transferTimeout, headers);
        if (offset > 0)
            args->extraHeaders["Range"] = "bytes=" + std::to_string(offset) + "-";
        bool writeError = false;
        size_t received = 0;
        args->onChunkCallback = [&out, &writeError, &received](const std::string& chunk)
        {
            out.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
            received += chunk.size();
            if (!out)
                writeError = true;
        };
        if (progress)
        {
            args->onProgressCallback = [&progress, offset](int current, int total) -> bool
            {
                progress(offset + (current > 0 ? static_cast<size_t>(current) : 0),
                         total > 0 ? offset + static_cast<size_t>(total) : 0);
                return true;
            };
        }

        const auto r = client.get(url, args);
        resp = toResponse(r);
        out.close();
        if (!out || writeError)
            return {0, "", "write failed for " + destPath + " (disk full?)"};

        // Only a 206 continues the file; anything else written this round is an
        // error body or a full copy that must not land behind the kept bytes.
        const int served = r ? r->statusCode : 0;
        const bool appended = offset > 0 ? served == 206 : served >= 200 && served < 300;
        const bool restart = offset > 0 && (served == 200 || served == 416);
        if (!appended)
        {
            std::filesystem::resize_file(destPath, restart ? 0 : offset, ec);
            if (ec)
                return {0, "", "cannot truncate " + destPath + ": " + ec.message()};
        }
        if (restart)
        {
            spdlog::warn("HTTP download resume refused (HTTP {}), restarting from zero", served);
            continue;
        }
        if (resp.statusCode != 0)
            return resp;

        // Lost connection: keep what arrived and resume; only rounds without
        // progress count toward the retry limit.
        failures = appended && received > 0 ? 0 : failures + 1;
        if (failures > kMaxRetries || attempt >= kMaxDownloadAttempts)
            return resp;
        const int delayMs = std::min(kMinWaitMs << std::max(failures - 1, 0), kMaxWaitMs);
        spdlog::warn("HTTP download interrupted at {} bytes (attempt {}/{}), resuming in {}ms",
                     offset + (appended ? received : 0), attempt, kMaxDownloadAttempts, delayMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

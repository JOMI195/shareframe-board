#pragma once
#include <functional>
#include <map>
#include <string>

struct HttpResponse
{
    int statusCode = 0;
    std::string body;
    std::string errorMsg;

    [[nodiscard]] bool ok() const { return statusCode >= 200 && statusCode < 300; }
};

class HTTPClient
{
public:
    using Headers = std::map<std::string, std::string>;

    explicit HTTPClient(int connectTimeoutSecs = 60, int transferTimeoutSecs = 60);

    [[nodiscard]] HttpResponse get(const std::string& url, const Headers& headers = {}) const;
    [[nodiscard]] HttpResponse post(const std::string& url, const std::string& body, const Headers& headers = {}) const;

    // Streams the response body to destPath (no in-memory copy). progress gets
    // (received, total) with total = Content-Length or 0 if unknown. An existing
    // destPath is treated as the start of the same file and resumed with a Range
    // request; on failure the partial file is kept for the next call.
    using ProgressFn = std::function<void(size_t received, size_t total)>;
    [[nodiscard]] HttpResponse downloadToFile(const std::string& url, const std::string& destPath,
                                              const Headers& headers = {},
                                              const ProgressFn& progress = nullptr) const;

private:
    // Retries the request while the server can't be reached, backing off
    // between attempts. Defined in the .cpp where it is instantiated.
    template <typename Fn>
    HttpResponse _withRetry(Fn&& doRequest) const;

    int _connectTimeout;
    int _transferTimeout;
};

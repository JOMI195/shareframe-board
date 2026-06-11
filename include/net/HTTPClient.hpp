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

    explicit HTTPClient(int connectTimeoutSecs = 30, int transferTimeoutSecs = 60);

    [[nodiscard]] HttpResponse get(const std::string& url, const Headers& headers = {}) const;
    [[nodiscard]] HttpResponse post(const std::string& url, const std::string& body, const Headers& headers = {}) const;

    // Streams the response body to destPath (no in-memory copy). progress gets
    // (received, total) with total = Content-Length or 0 if unknown. On any
    // failure the partial file is removed and the response carries the error.
    using ProgressFn = std::function<void(size_t received, size_t total)>;
    [[nodiscard]] HttpResponse downloadToFile(const std::string& url, const std::string& destPath,
                                              const Headers& headers = {},
                                              const ProgressFn& progress = nullptr) const;

private:
    int _connectTimeout;
    int _transferTimeout;
};

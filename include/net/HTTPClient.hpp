#pragma once
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

private:
    int _connectTimeout;
    int _transferTimeout;
};

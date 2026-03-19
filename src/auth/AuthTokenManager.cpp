#include "auth/AuthTokenManager.hpp"
#include "auth/HTTPAuth.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

static int64_t parseIso8601ToUnix(const std::string& s)
{
    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
        throw std::runtime_error("Cannot parse ISO8601 timestamp: " + s);
    return static_cast<int64_t>(timegm(&tm));
    // std::get_time stops at '.' or 'Z' — all 4 ISO 8601 variants handled
}

static bool isExpiredLocally(TokenRepository& repo)
{
    auto tokenOpt = repo.get();
    if (!tokenOpt.has_value())
    {
        spdlog::warn("No cached token found");
        return true;
    }
    return std::time(nullptr) >= tokenOpt->expiresAt;
}

static bool isExpiredServer(HTTPClient& http, const AppConfig& cfg, TokenRepository& repo)
{
    spdlog::info("Verifying token with server");
    auto tokenOpt = repo.get();
    if (!tokenOpt.has_value())
        return true;

    const nlohmann::json body = {{cfg.authToken.httpFetchTokenBodyKey, tokenOpt->value}};
    const std::string prot = cfg.production ? "https://" : "http://";
    const std::string url = prot + cfg.baseUrl + cfg.authToken.httpVerifyTokenUrl;

    const auto res = http.post(url, body.dump(), {{"Content-Type", "application/json"}});
    spdlog::info("Token verification result: {}", res.ok());
    return !res.ok();
}

static bool fetchAndSaveToken(const HTTPClient& http, TokenRepository& repo, const AppConfig& cfg)
{
    spdlog::info("Fetching new auth token");
    try
    {
        const std::string prot = cfg.production ? "https://" : "http://";
        const std::string url = prot + cfg.baseUrl + cfg.authToken.httpFetchTokenUrl;

        auto res = http.post(url, "{}", HTTPAuth::buildHTTPAuthHeaders(cfg));
        if (!res.ok())
        {
            spdlog::error("Failed to obtain token: HTTP {}", res.statusCode);
            return false;
        }
        auto j = nlohmann::json::parse(res.body);
        const Token token{
            j.at("access_token").get<std::string>(),
            parseIso8601ToUnix(j.at("expires_at").get<std::string>())
        };
        repo.save(token);
        spdlog::info("Token obtained. Expires at: {}", j.at("expires_at").get<std::string>());
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to obtain token: {}", e.what());
        return false;
    }
}

AuthTokenManager::AuthTokenManager(AppConfig& cfg, TokenRepository& repo, HTTPClient& http)
    : _cfg(cfg), _repo(repo), _http(http)
{
}

bool AuthTokenManager::init() const
{
    spdlog::info("Initializing token manager");
    return getOrFetchToken().has_value();
}

std::optional<std::string> AuthTokenManager::getOrFetchToken() const
{
    if (!isExpiredLocally(_repo) || !isExpiredServer(_http, _cfg, _repo))
        return _repo.get()->value;

    if (!fetchAndSaveToken(_http, _repo, _cfg))
        return std::nullopt;

    return _repo.get()->value;
}

void AuthTokenManager::invalidate() const
{
    spdlog::info("Token invalidated");
    _repo.clear();
}

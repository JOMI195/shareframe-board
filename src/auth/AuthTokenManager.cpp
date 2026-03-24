#include "auth/AuthTokenManager.hpp"
#include "auth/Ed25519Auth.hpp"
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
    const auto tokenOpt = repo.get();
    if (!tokenOpt.has_value())
    {
        spdlog::warn("No cached token found");
        return true;
    }
    return std::time(nullptr) >= tokenOpt->expiresAt;
}

static bool isExpiredServer(const HTTPClient& http, const AppConfig& cfg, TokenRepository& repo)
{
    spdlog::info("Verifying token with server");
    auto tokenOpt = repo.get();
    if (!tokenOpt.has_value())
        return true;

    const std::string httpFetchTokenBodyKey = "access_token";
    const nlohmann::json body = {{httpFetchTokenBodyKey, tokenOpt->value}};
    const std::string url = cfg.httpBaseUrl() + cfg.authToken.httpVerifyTokenUrl;

    const auto res = http.post(url, body.dump(), {{"Content-Type", "application/json"}});
    spdlog::info("Token verification result: {}", res.ok());
    return !res.ok();
}

static bool fetchAndSaveToken(const HTTPClient& http, TokenRepository& repo, const AppConfig& cfg)
{
    spdlog::info("Fetching new auth token");
    try
    {
        const std::string url = cfg.httpBaseUrl() + cfg.authToken.httpFetchTokenUrl;

        auto res = http.post(url, "{}", Ed25519Auth::buildHTTPAuthHeaders(cfg));
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

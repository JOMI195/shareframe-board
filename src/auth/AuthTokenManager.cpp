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

static bool acceptedByServer(const HTTPClient& http, const AppConfig& cfg, const Token& token)
{
    spdlog::info("Verifying token with server");
    const nlohmann::json body = {{"access_token", token.value}};
    const std::string url = cfg.httpBaseUrl() + cfg.authToken.httpVerifyTokenUrl;

    const auto res = http.post(url, body.dump(), {{"Content-Type", "application/json"}});
    spdlog::info("Token verification result: {}", res.ok());
    return res.ok();
}

enum class FetchResult
{
    Ok,
    Unregistered,
    Failed
};

static FetchResult fetchAndSaveToken(const HTTPClient& http, TokenRepository& repo, const AppConfig& cfg,
                                     std::string& claimCode)
{
    spdlog::info("Fetching new auth token");
    try
    {
        const std::string url = cfg.httpBaseUrl() + cfg.authToken.httpFetchTokenUrl;

        auto res = http.post(url, "{}", Ed25519Auth::buildHTTPAuthHeaders(cfg));
        if (!res.ok())
        {
            if (const auto body = nlohmann::json::parse(res.body, nullptr, false);
                body.is_object() && body.value("code", "") == "frame_unregistered")
            {
                spdlog::warn("Server refuses frame {}: not registered to a user", cfg.frameId);
                if (const auto it = body.find("claim_code"); it != body.end() && it->is_string())
                    claimCode = it->get<std::string>();
                return FetchResult::Unregistered;
            }
            spdlog::error("Failed to obtain token: HTTP {}", res.statusCode);
            return FetchResult::Failed;
        }
        auto j = nlohmann::json::parse(res.body);
        const Token token{
            j.at("access_token").get<std::string>(),
            parseIso8601ToUnix(j.at("expires_at").get<std::string>())
        };
        repo.save(token);
        spdlog::info("Token obtained. Expires at: {}", j.at("expires_at").get<std::string>());
        return FetchResult::Ok;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to obtain token: {}", e.what());
        return FetchResult::Failed;
    }
}

AuthTokenManager::AuthTokenManager(AppConfig& cfg, TokenRepository& repo, HTTPClient& http)
    : _cfg(cfg), _repo(repo), _http(http)
{
}

std::optional<std::string> AuthTokenManager::getOrFetchToken() const
{
    // Other services share and may clear the stored token at any time: read it once.
    if (const auto token = _repo.get())
    {
        if (std::time(nullptr) < token->expiresAt || acceptedByServer(_http, _cfg, *token))
            return token->value;
    }
    else
        spdlog::warn("No cached token found");

    std::string claimCode;
    const auto result = fetchAndSaveToken(_http, _repo, _cfg, claimCode);
    {
        std::lock_guard lk(_claimMtx);
        _claimCode = std::move(claimCode);
    }
    _unregistered.store(result == FetchResult::Unregistered);
    if (result != FetchResult::Ok)
        return std::nullopt;

    const auto fetched = _repo.get();
    return fetched ? std::optional(fetched->value) : std::nullopt;
}

std::string AuthTokenManager::claimCode() const
{
    std::lock_guard lk(_claimMtx);
    return _claimCode;
}

void AuthTokenManager::invalidate() const
{
    spdlog::info("Token invalidated");
    _repo.clear();
}

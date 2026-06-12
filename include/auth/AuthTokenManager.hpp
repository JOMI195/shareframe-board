#pragma once
#include "config/AppConfig.hpp"
#include "repository/TokenRepository.hpp"
#include "net/HTTPClient.hpp"
#include <optional>
#include <string>

/// Manages the frame access token lifecycle: local expiry checks, server
/// verification, and fetching a new token via HTTP auth when needed.
/// Tokens are persisted in the TokenRepository between runs.
class AuthTokenManager
{
public:
    explicit AuthTokenManager(AppConfig& cfg, TokenRepository& repo, HTTPClient& http);

    /// Returns the cached token if still valid, or fetches and caches a new one.
    std::optional<std::string> getOrFetchToken() const;

    /// Clears the cached token; call on 401 so the next fetch gets a fresh one.
    void invalidate() const;

private:
    AppConfig& _cfg;
    TokenRepository& _repo;
    HTTPClient& _http;
};

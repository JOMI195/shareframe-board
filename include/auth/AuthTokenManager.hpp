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
    /// @param cfg   Application config — provides token URLs and auth secrets.
    /// @param repo  Persistent token store — used to cache and retrieve tokens.
    /// @param http  HTTP client — used for token fetch and server verification.
    explicit AuthTokenManager(AppConfig& cfg, TokenRepository& repo, HTTPClient& http);

    /// Returns the cached token if still valid, or fetches and caches a new one.
    /// @return Token value on success, nullopt if the fetch fails.
    std::optional<std::string> getOrFetchToken() const;

    /// Clears the cached token. Call when a request returns 401 so the next
    /// getOrFetchToken() will re-fetch a fresh token from the server.
    void invalidate() const;

private:
    AppConfig& _cfg;
    TokenRepository& _repo;
    HTTPClient& _http;
};

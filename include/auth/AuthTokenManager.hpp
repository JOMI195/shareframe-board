#pragma once
#include "config/AppConfig.hpp"
#include "repository/TokenRepository.hpp"
#include "net/HTTPClient.hpp"
#include <atomic>
#include <mutex>
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

    /// Whether the last fetch was refused because nobody owns the frame yet.
    [[nodiscard]] bool isUnregistered() const { return _unregistered.load(); }

    /// The code the server handed out with the last refusal; empty when it sent none.
    [[nodiscard]] std::string claimCode() const;

private:
    AppConfig& _cfg;
    TokenRepository& _repo;
    HTTPClient& _http;
    mutable std::atomic<bool> _unregistered{false};
    mutable std::mutex _claimMtx;
    mutable std::string _claimCode;
};

#pragma once
#include "AuthTokenManager.hpp"
#include "util/NonInstantiable.hpp"
#include <map>
#include <string>

/// Static utility for building token-based Authorization headers.
/// Delegates token retrieval and refresh to AuthTokenManager.
class TokenAuth : NonInstantiable
{
public:
    /// Builds the Authorization header for token-authenticated API requests.
    /// Calls getOrFetchToken() internally — triggers a fetch if the token is
    /// expired or missing.
    /// @param tokenManager  Manager used to retrieve or refresh the access token.
    /// @return Map containing {"Authorization": "Frame-Access-Token <token>"},
    ///         or an empty map if no valid token could be obtained.
    static std::map<std::string, std::string> buildTokenAuthHeaders(AuthTokenManager& tokenManager);
};

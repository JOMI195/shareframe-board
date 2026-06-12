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
    /// Builds the Authorization header (fetches/refreshes the token as needed);
    /// empty map if no valid token could be obtained.
    static std::map<std::string, std::string> buildTokenAuthHeaders(const AuthTokenManager& tokenManager);
};

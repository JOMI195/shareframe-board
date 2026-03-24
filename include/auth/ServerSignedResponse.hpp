#pragma once
#include <string>

#include "config/AppConfig.hpp"
#include "util/NonInstantiable.hpp"

/// Verifies Ed25519-signed responses from the server.
/// Only verification is needed on the board side — signing is server-only.
class ServerSignedResponse : NonInstantiable
{
public:
    /// Verifies a signed response and returns the "data" payload as a JSON string.
    /// @param signedResponse  Base64url-encoded envelope from server
    /// @param config          App config containing server public key and own frame ID
    /// @param maxAgeSecs      Maximum age of the response in seconds (default 300)
    /// @return JSON string of the "data" field
    /// @throws std::runtime_error on verification failure
    static std::string verify(
        const std::string& signedResponse,
        const AppConfig& config,
        int maxAgeSecs = 300
    );
};

#pragma once
#include <map>
#include <string>

#include "config/AppConfig.hpp"
#include "util/NonInstantiable.hpp"

class Ed25519Auth : NonInstantiable
{
public:
    static std::map<std::string, std::string> buildHTTPAuthHeaders(
        const AppConfig& config
    );

private:
    static std::string sign(
        const std::string& message,
        const std::string& base64PrivateKey
    );
};

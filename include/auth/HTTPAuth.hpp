#pragma once
#include <string>
#include <map>

#include "config/AppConfig.hpp"
#include "util/NonInstantiable.hpp"

class HTTPAuth : NonInstantiable
{
public:
    static std::map<std::string, std::string> buildHTTPAuthHeaders(
        const AppConfig& config
    );

private:
    static std::string generateHTTPAuthHash(
        const std::string& privateSerialNumber,
        const std::string& timestamp,
        const std::string& secretKey
    );
};

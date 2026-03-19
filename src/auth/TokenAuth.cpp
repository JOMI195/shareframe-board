#include "auth/TokenAuth.hpp"
#include <spdlog/spdlog.h>

std::map<std::string, std::string> TokenAuth::buildTokenAuthHeaders(
    AuthTokenManager& tokenManager
)
{
    auto token = tokenManager.getOrFetchToken();
    if (!token.has_value())
    {
        spdlog::error("Cannot build token auth headers: no valid token");
        return {};
    }
    return {{"Authorization", "Frame-Access-Token " + *token}};
}

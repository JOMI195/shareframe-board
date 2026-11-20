#pragma once
#include <cctype>
#include <string>
#include <unordered_map>
#include <utility>

namespace HttpUtil {

/// Split a URI like "/api/logs?service_name=foo&lines=100" into
/// path "/api/logs" and query params {"service_name":"foo", "lines":"100"}.
inline std::pair<std::string, std::unordered_map<std::string, std::string>>
parseUri(const std::string& uri)
{
    auto qpos = uri.find('?');
    if (qpos == std::string::npos)
        return {uri, {}};

    std::string path = uri.substr(0, qpos);
    std::string query = uri.substr(qpos + 1);

    std::unordered_map<std::string, std::string> params;
    size_t start = 0;
    while (start < query.size())
    {
        size_t amp = query.find('&', start);
        if (amp == std::string::npos)
            amp = query.size();

        std::string pair = query.substr(start, amp - start);
        if (auto eq = pair.find('='); eq != std::string::npos)
            params[pair.substr(0, eq)] = pair.substr(eq + 1);
        else if (!pair.empty())
            params[pair] = "";

        start = amp + 1;
    }

    return {path, params};
}

/// The host of an Origin ("http://Host:8090") or Host ("host:8090") header, lower-cased, without the port.
inline std::string hostOf(std::string value)
{
    if (const auto scheme = value.find("://"); scheme != std::string::npos)
        value.erase(0, scheme + 3);
    value = value.substr(0, value.find('/'));
    if (value.starts_with('['))
        value = value.substr(0, value.find(']') + 1);
    else if (const auto colon = value.find(':'); colon != std::string::npos)
        value.erase(colon);
    for (auto& c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

} // namespace HttpUtil

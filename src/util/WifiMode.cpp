#include "util/WifiMode.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <spdlog/spdlog.h>

namespace
{
constexpr std::array<std::string_view, 2> kProtectedNetworks = {"preconfigured", "Jomi"};
}

nlohmann::json WifiMode::read(const std::string& path)
{
    if (std::ifstream f(path); f)
    {
        try
        {
            nlohmann::json j;
            f >> j;
            if (j.is_object())
                return j;
        }
        catch (const std::exception& e)
        {
            static const auto logger = spdlog::default_logger()->clone("WifiMode");
            logger->warn("failed to parse {}: {}", path, e.what());
        }
    }

    return {
        {"mode", "connecting"}, {"ssid", ""}, {"internet", false}, {"known_ssids", nlohmann::json::array()},
        {"ap_ssid", ""}, {"ap_password", ""}
    };
}

bool WifiMode::isProtectedNetwork(const std::string_view ssid)
{
    return std::ranges::any_of(kProtectedNetworks, [&](const std::string_view p)
    {
        return std::ranges::equal(p, ssid, [](const char a, const char b)
        {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
    });
}


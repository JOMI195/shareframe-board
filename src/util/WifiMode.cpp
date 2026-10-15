#include "util/WifiMode.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

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
        {"mode", "connecting"}, {"ssid", ""}, {"internet", false},
        {"ap_ssid", ""}, {"ap_password", ""}
    };
}

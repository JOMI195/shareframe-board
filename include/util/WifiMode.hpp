#pragma once
#include "util/NonInstantiable.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

class WifiMode : NonInstantiable
{
public:
    /// The mode published by the wifi-mode-manager daemon; `connecting` while it is
    /// missing or unreadable (early boot, a write in progress).
    [[nodiscard]] static nlohmann::json read(const std::string& path);

    /// Factory networks the frame ships with, which the owner can't change or forget.
    [[nodiscard]] static bool isProtectedNetwork(std::string_view ssid);

    /// Saved networks in the mode's `known_ssids`, not counting the protected ones.
    [[nodiscard]] static int userNetworkCount(const nlohmann::json& mode);
};

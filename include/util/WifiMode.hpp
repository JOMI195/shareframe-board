#pragma once
#include "util/NonInstantiable.hpp"
#include <nlohmann/json.hpp>
#include <string>

class WifiMode : NonInstantiable
{
public:
    /// The mode published by the wifi-mode-manager daemon; `connecting` while it is
    /// missing or unreadable (early boot, a write in progress).
    [[nodiscard]] static nlohmann::json read(const std::string& path);
};

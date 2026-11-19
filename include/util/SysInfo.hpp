#pragma once
#include "util/NonInstantiable.hpp"
#include <nlohmann/json.hpp>
#include <string>

class SysInfo : NonInstantiable
{
public:
    /// `shareframe-sysinfo` output (`key=value` lines) as JSON. Metrics become numbers,
    /// `true`/`false` booleans; everything else stays a string, so identifiers such as a
    /// serial `000123` or an all-digit sha keep their exact text.
    [[nodiscard]] static nlohmann::json parse(const std::string& output);
};

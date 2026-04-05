#pragma once
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class WifiManager
{
public:
    WifiManager();

    [[nodiscard]] nlohmann::json getCurrentConnection() const;
    [[nodiscard]] nlohmann::json getSavedNetworks() const;
    nlohmann::json connect(const std::string& ssid, const std::string& password) const;
    nlohmann::json forget(const std::string& ssid) const;
    nlohmann::json rename(const std::string& oldName, const std::string& newName) const;

private:
    [[nodiscard]] bool _isProtected(const std::string& name) const;
    [[nodiscard]] std::string _aliasIfProtected(const std::string& name) const;

    std::shared_ptr<spdlog::logger> logger_;
    static const std::vector<std::string> protectedNetworks_;
    static constexpr const char* protectedAlias_ = "VOREINGESTELLT";
};

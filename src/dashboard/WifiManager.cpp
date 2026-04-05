#include "dashboard/WifiManager.hpp"
#include "util/Subprocess.hpp"
#include <algorithm>
#include <sstream>

const std::vector<std::string> WifiManager::protectedNetworks_ = {"preconfigured", "Jomi"};

WifiManager::WifiManager()
    : logger_(spdlog::default_logger()->clone("WifiManager"))
{
}

bool WifiManager::_isProtected(const std::string& name) const
{
    return std::ranges::any_of(protectedNetworks_, [&](const std::string& p)
    {
        return std::ranges::equal(p, name, [](char a, char b)
        {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
    });
}

std::string WifiManager::_aliasIfProtected(const std::string& name) const
{
    return _isProtected(name) ? protectedAlias_ : name;
}

nlohmann::json WifiManager::getCurrentConnection() const
{
    auto result = Subprocess::run(
        {"nmcli", "-t", "-f", "NAME,DEVICE", "connection", "show", "--active"}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("nmcli failed: {}", result.stdErr);
        return {{"error", "Failed to query connections"}};
    }

    // Parse terse output: "NAME:DEVICE\n"
    std::istringstream stream(result.stdOut);
    std::string line;
    while (std::getline(stream, line))
    {
        auto sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string name = line.substr(0, sep);
        std::string device = line.substr(sep + 1);

        if (device == "wlan0")
        {
            return {
                {"connection_name", _aliasIfProtected(name)},
                {"is_protected", _isProtected(name)}
            };
        }
    }

    return {{"connection_name", ""}, {"is_protected", false}};
}

nlohmann::json WifiManager::getSavedNetworks() const
{
    auto result = Subprocess::run(
        {"nmcli", "-t", "-f", "TYPE,NAME", "connection", "show"}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("nmcli failed: {}", result.stdErr);
        return {{"error", "Failed to query saved networks"}};
    }

    std::vector<std::string> networks;
    std::istringstream stream(result.stdOut);
    std::string line;
    while (std::getline(stream, line))
    {
        auto sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string type = line.substr(0, sep);
        std::string name = line.substr(sep + 1);

        if (type == "802-11-wireless" && !_isProtected(name))
            networks.push_back(name);
    }

    return {{"networks", networks}};
}

nlohmann::json WifiManager::connect(const std::string& ssid, const std::string& password) const
{
    if (_isProtected(ssid))
        return {{"error", "Cannot modify protected network"}};

    auto result = Subprocess::run(
        {"sudo", "nmcli", "connection", "add",
         "type", "wifi",
         "con-name", ssid,
         "ifname", "wlan0",
         "ssid", ssid,
         "wifi-sec.key-mgmt", "wpa-psk",
         "wifi-sec.psk", password}, 15);

    if (result.exitCode != 0)
    {
        logger_->error("nmcli connect failed: {}", result.stdErr);
        return {{"error", "Failed to add WiFi connection"}};
    }

    // Activate the new connection
    auto activate = Subprocess::run(
        {"sudo", "nmcli", "connection", "up", ssid}, 30);

    if (activate.exitCode != 0)
    {
        logger_->error("nmcli activate failed: {}", activate.stdErr);
        return {{"error", "Connection added but activation failed"}};
    }

    logger_->info("Connected to WiFi: {}", ssid);
    return {{"message", "Connected to " + ssid}};
}

nlohmann::json WifiManager::forget(const std::string& ssid) const
{
    if (_isProtected(ssid))
        return {{"error", "Cannot delete protected network"}};

    // Check if this is the active connection
    auto current = getCurrentConnection();
    if (current.value("connection_name", "") == ssid)
        return {{"error", "Cannot delete active connection"}};

    auto result = Subprocess::run(
        {"sudo", "nmcli", "connection", "delete", ssid}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("nmcli delete failed: {}", result.stdErr);
        return {{"error", "Failed to delete connection"}};
    }

    logger_->info("Forgot WiFi: {}", ssid);
    return {{"message", "Connection deleted"}};
}

nlohmann::json WifiManager::rename(const std::string& oldName, const std::string& newName) const
{
    if (_isProtected(oldName) || _isProtected(newName))
        return {{"error", "Cannot rename protected network"}};

    auto result = Subprocess::run(
        {"sudo", "nmcli", "connection", "modify", oldName, "connection.id", newName}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("nmcli rename failed: {}", result.stdErr);
        return {{"error", "Failed to rename connection"}};
    }

    logger_->info("Renamed WiFi: {} -> {}", oldName, newName);
    return {{"message", "Connection renamed"}};
}

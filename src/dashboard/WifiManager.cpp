#include "dashboard/WifiManager.hpp"
#include "util/Subprocess.hpp"
#include "util/WifiMode.hpp"
#include "util/WpaText.hpp"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

WifiManager::WifiManager(std::string wifiModePath)
    : logger_(spdlog::default_logger()->clone("WifiManager")), wifiModePath_(std::move(wifiModePath))
{
}

std::string WifiManager::_aliasIfProtected(const std::string& name) const
{
    return WifiMode::isProtectedNetwork(name) ? protectedAlias_ : name;
}

nlohmann::json WifiManager::getCurrentConnection() const
{
    // `shareframe-wifi status` wraps `wpa_cli status`: parse ssid= + wpa_state=.
    auto result = Subprocess::run({"shareframe-wifi", "status"}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi status failed: {}", result.stdErr);
        return {{"error", "Failed to query connection"}};
    }

    std::string ssid;
    bool completed = false;
    std::istringstream stream(result.stdOut);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.rfind("ssid=", 0) == 0)
            ssid = WpaText::decode(line.substr(5));
        else if (line.rfind("wpa_state=", 0) == 0)
            completed = (line.substr(10) == "COMPLETED");
    }

    if (!completed || ssid.empty())
        return {{"connection_name", ""}, {"is_protected", false}};

    return {
        {"connection_name", _aliasIfProtected(ssid)},
        {"is_protected", WifiMode::isProtectedNetwork(ssid)}
    };
}

nlohmann::json WifiManager::getSavedNetworks() const
{
    // `shareframe-wifi list` wraps `wpa_cli list_networks`: a header row then
    // tab-separated "id<TAB>ssid<TAB>bssid<TAB>flags" rows.
    auto result = Subprocess::run({"shareframe-wifi", "list"}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi list failed: {}", result.stdErr);
        return {{"error", "Failed to query saved networks"}};
    }

    std::vector<std::string> networks;
    std::istringstream stream(result.stdOut);
    std::string line;
    bool header = true;
    while (std::getline(stream, line))
    {
        if (header) { header = false; continue; }  // drop "network id / ssid / ..."

        auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        auto t2 = line.find('\t', t1 + 1);
        std::string ssid = WpaText::decode(line.substr(
            t1 + 1, (t2 == std::string::npos ? line.size() : t2) - (t1 + 1)));

        if (ssid.empty() || WifiMode::isProtectedNetwork(ssid)) continue;
        networks.push_back(ssid);
    }

    return {{"networks", networks}};
}

nlohmann::json WifiManager::connect(const std::string& ssid, const std::string& password) const
{
    if (WifiMode::isProtectedNetwork(ssid))
        return {{"error", "Protected network"}, {"status", 409}};

    // `add` hashes the PSK, enables the network and saves the config; wpa
    // associates on its own afterwards.
    auto result = Subprocess::run({"shareframe-wifi", "add", ssid, password}, 20);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi add failed: {}", result.stdErr);
        return {{"error", "Failed to add WiFi network"}};
    }

    logger_->info("Added WiFi network: {}", ssid);
    return {{"message", "Saved " + ssid}};
}

nlohmann::json WifiManager::getScan() const
{
    const auto path = std::filesystem::path(wifiModePath_).parent_path() / "wifi-scan.json";
    nlohmann::json scan = {{"networks", nlohmann::json::array()}, {"age_secs", nullptr}};
    std::ifstream f(path);
    if (!f)
        return scan;
    try
    {
        const auto raw = nlohmann::json::parse(f);
        for (const auto& n : raw.at("networks"))
            scan["networks"].push_back({{"ssid", WpaText::decode(n.at("ssid").get<std::string>())},
                                        {"signal", n.at("signal").get<int>()},
                                        {"secured", n.at("secured").get<bool>()}});
        scan["age_secs"] = std::max<int64_t>(0, std::time(nullptr) - raw.at("at").get<int64_t>());
    }
    catch (const nlohmann::json::exception& e)
    {
        logger_->warn("Unreadable {}: {}", path.string(), e.what());
        return {{"networks", nlohmann::json::array()}, {"age_secs", nullptr}};
    }
    return scan;
}

bool WifiManager::isApMode() const
{
    return getWifiMode().value("mode", "") == "ap";
}

nlohmann::json WifiManager::requestConnect() const
{
    // wifi-mode-manager polls for this flag while it hosts the AP and then tries the saved networks.
    const auto flag = std::filesystem::path(wifiModePath_).parent_path() / "wifi-retry-requested";
    if (std::ofstream retryFlag{flag}; !retryFlag)
    {
        logger_->error("Could not create {}", flag.string());
        return {{"error", "Failed to request a connection"}};
    }
    logger_->info("Requested leaving the AP for the saved networks");
    return {{"message", "Connecting"}};
}

nlohmann::json WifiManager::getWifiMode() const
{
    return WifiMode::read(wifiModePath_);
}

nlohmann::json WifiManager::forget(const std::string& ssid) const
{
    if (WifiMode::isProtectedNetwork(ssid))
        return {{"error", "Protected network"}, {"status", 409}};

    // Refuse to drop the network we're currently associated with.
    auto current = getCurrentConnection();
    if (current.value("connection_name", "") == ssid)
        return {{"error", "Active network"}, {"status", 409}};

    auto result = Subprocess::run({"shareframe-wifi", "del", ssid}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi del failed: {}", result.stdErr);
        return {{"error", "Failed to delete network"}};
    }

    logger_->info("Forgot WiFi: {}", ssid);
    return {{"message", "Connection deleted"}};
}

nlohmann::json WifiManager::setApPassword(const std::string& password) const
{
    auto result = Subprocess::run({"shareframe-ap", "setpass", password}, 20);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-ap setpass failed: {}", result.stdErr);
        return {{"error", "Failed to set AP password"}};
    }

    logger_->info("AP password changed");
    return {{"message", "AP-Passwort geändert"}};
}

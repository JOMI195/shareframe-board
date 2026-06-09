#include "dashboard/WifiManager.hpp"
#include "util/Subprocess.hpp"
#include <algorithm>
#include <fstream>
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
            ssid = line.substr(5);
        else if (line.rfind("wpa_state=", 0) == 0)
            completed = (line.substr(10) == "COMPLETED");
    }

    if (!completed || ssid.empty())
        return {{"connection_name", ""}, {"is_protected", false}};

    return {
        {"connection_name", _aliasIfProtected(ssid)},
        {"is_protected", _isProtected(ssid)}
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
        std::string ssid = line.substr(
            t1 + 1, (t2 == std::string::npos ? line.size() : t2) - (t1 + 1));

        if (ssid.empty() || _isProtected(ssid)) continue;
        networks.push_back(ssid);
    }

    return {{"networks", networks}};
}

nlohmann::json WifiManager::connect(const std::string& ssid, const std::string& password) const
{
    if (_isProtected(ssid))
        return {{"error", "Cannot modify protected network"}};

    // `add` hashes the PSK, enables the network and saves the config; wpa
    // associates on its own afterwards.
    auto result = Subprocess::run({"shareframe-wifi", "add", ssid, password}, 20);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi add failed: {}", result.stdErr);
        return {{"error", "Failed to add WiFi network"}};
    }

    logger_->info("Added WiFi network: {}", ssid);

    // Nudge wifi-mode-manager to retry the station immediately. This matters in
    // AP fallback mode: it switches back to the new network at once instead of
    // waiting for the periodic retry or a manual reboot. No-op (file just sits)
    // when already connected. /run/shareframe is created by the daemon.
    if (std::ofstream retryFlag{"/run/shareframe/wifi-retry-requested"}; !retryFlag)
        logger_->warn("could not create wifi-retry-requested flag");

    return {{"message", "Connected to " + ssid}};
}

nlohmann::json WifiManager::getWifiMode() const
{
    std::ifstream f("/run/shareframe/wifi-mode.json");
    if (f)
    {
        try
        {
            nlohmann::json j;
            f >> j;
            return j;
        }
        catch (const std::exception& e)
        {
            logger_->warn("failed to parse wifi-mode.json: {}", e.what());
        }
    }

    // Daemon hasn't published yet (early boot) — report a benign default.
    return {
        {"mode", "connecting"}, {"ssid", ""}, {"internet", false},
        {"ap_ssid", ""}, {"ap_password", ""}
    };
}

nlohmann::json WifiManager::forget(const std::string& ssid) const
{
    if (_isProtected(ssid))
        return {{"error", "Cannot delete protected network"}};

    // Refuse to drop the network we're currently associated with.
    auto current = getCurrentConnection();
    if (current.value("connection_name", "") == ssid)
        return {{"error", "Cannot delete active connection"}};

    auto result = Subprocess::run({"shareframe-wifi", "del", ssid}, 10);

    if (result.exitCode != 0)
    {
        logger_->error("shareframe-wifi del failed: {}", result.stdErr);
        return {{"error", "Failed to delete network"}};
    }

    logger_->info("Forgot WiFi: {}", ssid);
    return {{"message", "Connection deleted"}};
}

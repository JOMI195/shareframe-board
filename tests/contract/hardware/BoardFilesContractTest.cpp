#include "config/AppConfig.hpp"
#include "config/ConfigLoader.hpp"
#include "config/Profile.hpp"
#include "dashboard/WifiManager.hpp"
#include "support/Golden.hpp"
#include "support/TempDir.hpp"
#include <gtest/gtest.h>

// automation.sh builds an image for a profile; the apps must load exactly those.
TEST(BoardFilesContract, ProfilesAreTheOnesTheImageBuildAccepts)
{
    std::string names;
    for (const auto p : kProfiles)
    {
        names += std::string(profileName(p)) + "\n";
        EXPECT_EQ(ConfigLoader::parseProfile(std::string(profileName(p))), p);
    }
    golden::expectBoard("profiles.txt", names);
}

// Flags shared with the image's scripts: wifi-mode-manager and rauc-mark-good watch the first ones,
// wifi-mode-manager and a system migration leave the last two for the display.
TEST(BoardFilesContract, FlagsLandWhereTheScriptsLook)
{
    const AppConfig cfg;
    TempDir dir;
    WifiManager wifi{(dir.path() / "wifi-mode.json").string()};
    wifi.requestConnect();
    std::string retryName;
    for (const auto& e : std::filesystem::directory_iterator(dir.path()))
        retryName = e.path().filename().string();

    const auto runDir = std::filesystem::path(cfg.display.wifiModePath).parent_path();
    golden::expectBoard("run-flags.txt", "wifi_mode=" + cfg.display.wifiModePath + "\n" +
                                             "wifi_retry_requested=" + (runDir / retryName).string() + "\n" +
                                             "backend_ok=" + cfg.heartbeat.backendOkPath + "\n" +
                                             "wifi_joined=" + cfg.display.wifiJoinedPath + "\n" +
                                             "registered_once=" + cfg.display.registeredOncePath + "\n");
}

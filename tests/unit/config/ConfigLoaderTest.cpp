#include "config/ConfigLoader.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
void write(const std::filesystem::path& p, const std::string& content)
{
    std::ofstream(p) << content;
}

class ConfigLoaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        write(dir / "config.toml", R"(
base_dir = "/base"
base_url = "example.org"
[display]
interval_secs = 900
night_start_hour = 2
night_end_hour = 5
[database]
database_path = "db"
)");
        write(dir / "secrets.toml", "dashboard_initial_password = \"pw\"\n");
    }

    AppConfig load(Profile p = Profile::Dev)
    {
        return ConfigLoader::load(p, (dir / "config.toml").string(), (dir / "secrets.toml").string(),
                                  (dir / "shareframe-release").string());
    }

    TempDir dir;
};
}

TEST_F(ConfigLoaderTest, BaseConfigWithoutProfileFile)
{
    const auto cfg = load();
    EXPECT_EQ(cfg.baseUrl, "example.org");
    EXPECT_EQ(cfg.display.intervalSecs, 900);
    EXPECT_EQ(cfg.secrets.dashboardInitialPassword, "pw");
    EXPECT_EQ(cfg.version, "dev");
}

TEST_F(ConfigLoaderTest, ProfileOverlayOnlyOverridesPresentKeys)
{
    write(dir / "config.prod.toml", "[display]\ninterval_secs = 1800\n");
    const auto cfg = load(Profile::Prod);
    EXPECT_EQ(cfg.display.intervalSecs, 1800);
    EXPECT_EQ(cfg.display.nightStartHour, 2);
    EXPECT_EQ(cfg.baseUrl, "example.org");
}

TEST_F(ConfigLoaderTest, OverlayOfOtherProfileIsIgnored)
{
    write(dir / "config.prod.toml", "[display]\ninterval_secs = 1800\n");
    EXPECT_EQ(load(Profile::Dev).display.intervalSecs, 900);
}

TEST_F(ConfigLoaderTest, RelativePathsResolveAgainstBaseDir)
{
    const auto cfg = load();
    EXPECT_EQ(cfg.database.databasePath, "/base/db");
    EXPECT_EQ(cfg.image.imageSavePath, "/base/images");
}

TEST_F(ConfigLoaderTest, VersionIsReadFromReleaseFile)
{
    write(dir / "shareframe-release", "OS_SHA=abc1234\nVERSION=1.4.2\nAPP_SHA=def5678\n");
    EXPECT_EQ(load().version, "1.4.2");
}

TEST_F(ConfigLoaderTest, ReleaseFileWithoutVersionFallsBackToDev)
{
    write(dir / "shareframe-release", "OS_SHA=abc1234\nVERSION=\n");
    EXPECT_EQ(load().version, "dev");
}

TEST_F(ConfigLoaderTest, ValidationListsEveryProblem)
{
    write(dir / "config.dev.toml", "base_url = \"\"\n[display]\nnight_start_hour = 4\nnight_end_hour = 4\n");
    try
    {
        load();
        FAIL() << "expected validation error";
    }
    catch (const std::runtime_error& e)
    {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("base_url"), std::string::npos) << msg;
        EXPECT_NE(msg.find("must differ"), std::string::npos) << msg;
    }
}

TEST_F(ConfigLoaderTest, MissingFilesAndBadTomlThrow)
{
    EXPECT_THROW(ConfigLoader::load(Profile::Dev, (dir / "nope.toml").string(),
                                    (dir / "secrets.toml").string(), ""), std::runtime_error);
    write(dir / "config.toml", "this is = = not toml");
    EXPECT_THROW(load(), std::runtime_error);
}

TEST(ConfigLoader, ParseProfile)
{
    EXPECT_EQ(ConfigLoader::parseProfile("dev"), Profile::Dev);
    EXPECT_EQ(ConfigLoader::parseProfile("prod"), Profile::Prod);
    EXPECT_EQ(ConfigLoader::parseProfile("prod.local"), Profile::ProdLocal);
    EXPECT_THROW(ConfigLoader::parseProfile("staging"), std::invalid_argument);
}

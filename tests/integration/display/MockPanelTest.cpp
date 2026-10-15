#include "display/DisplayManager.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/TestDb.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>

namespace
{
class MockPanelTest : public ::testing::Test
{
protected:
    MockPanelTest()
    {
        cfg.display.mockDisplay = true;
        cfg.display.minRefreshSecs = 0;
        cfg.display.mockOutputDir = (db.dir() / "panel").string();
    }

    std::string read(const std::string& name) const
    {
        std::ifstream f(std::filesystem::path(cfg.display.mockOutputDir) / name, std::ios::binary);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    nlohmann::json meta() const { return nlohmann::json::parse(read("panel.json")); }

    TestDb db;
    AppConfig cfg;
    DisplayMetricsRepository metrics{db.get()};
};

const std::string kHeader = "P4\n800 480\n";
const std::filesystem::path kLoadingImage =
    std::filesystem::path(SHAREFRAME_ASSETS_DIR) / "image/loading/logo-frame-loading-shareframe.jpg";
}

TEST_F(MockPanelTest, DisplayedImageIsWrittenAsOneBitFrame)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(display.displayImage(kLoadingImage));

    const auto pbm = read("panel.pbm");
    ASSERT_EQ(pbm.size(), kHeader.size() + DisplayManager::BUFFER_SIZE);
    EXPECT_EQ(pbm.substr(0, kHeader.size()), kHeader);
    const auto body = pbm.substr(kHeader.size());
    EXPECT_NE(body.find_first_not_of('\0'), std::string::npos) << "frame is all white";
    EXPECT_NE(body.find_first_not_of('\xFF'), std::string::npos) << "frame is all black";

    EXPECT_EQ(meta()["state"], "image");
    EXPECT_EQ(meta()["source"], kLoadingImage.string());
    EXPECT_EQ(meta()["seq"], 1);
}

TEST_F(MockPanelTest, ClearWritesWhiteFrame)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(display.displayImage(kLoadingImage));
    display.clear();

    const auto body = read("panel.pbm").substr(kHeader.size());
    EXPECT_EQ(body, std::string(DisplayManager::BUFFER_SIZE, '\0'));
    EXPECT_EQ(meta()["state"], "cleared");
    EXPECT_EQ(meta()["seq"], 2);
}

TEST_F(MockPanelTest, CountsRefreshesLikeThePanel)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(display.displayImage(kLoadingImage));
    ASSERT_TRUE(display.displayImage(kLoadingImage));
    display.clear();

    EXPECT_EQ(metrics.get("epd_refresh_total"), 3);
    EXPECT_EQ(metrics.get("epd_image_refresh_total"), 2);
    EXPECT_EQ(metrics.get("epd_clear_total"), 1);
    EXPECT_GT(metrics.get("epd_first_use_at"), 0);
}

TEST_F(MockPanelTest, UndecodableImageFailsLikeHardware)
{
    const auto bogus = db.dir() / "bogus.jpg";
    std::ofstream(bogus) << "not an image";
    DisplayManager display(cfg, metrics);
    EXPECT_FALSE(display.displayImage(bogus));
}

TEST_F(MockPanelTest, SameImageGivesIdenticalFrame)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(display.displayImage(kLoadingImage));
    const auto first = read("panel.pbm");
    ASSERT_TRUE(display.displayImage(kLoadingImage));
    EXPECT_EQ(read("panel.pbm"), first);
}

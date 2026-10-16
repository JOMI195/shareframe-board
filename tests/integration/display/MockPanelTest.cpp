#include "display/DisplayManager.hpp"
#include "display/PhotoRenderer.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/TestDb.hpp"
#include "support/Wait.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <thread>

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

    static bool paint(DisplayManager& display, const std::filesystem::path& image)
    {
        return display.displayBuffer(PhotoRenderer::render(image), image.string());
    }

    TestDb db;
    AppConfig cfg;
    DisplayMetricsRepository metrics{db.get()};
};

const std::string kHeader = "P4\n800 480\n";
const std::filesystem::path kLoadingImage =
    std::filesystem::path(SHAREFRAME_ASSETS_DIR) / "image/loading/logo-frame-generic-shareframe.jpg";
}

TEST_F(MockPanelTest, DisplayedImageIsWrittenAsOneBitFrame)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));

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
    ASSERT_TRUE(paint(display, kLoadingImage));
    display.clear();

    const auto body = read("panel.pbm").substr(kHeader.size());
    EXPECT_EQ(body, std::string(DisplayManager::BUFFER_SIZE, '\0'));
    EXPECT_EQ(meta()["state"], "cleared");
    EXPECT_EQ(meta()["seq"], 2);
}

TEST_F(MockPanelTest, CountsRefreshesLikeThePanel)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));
    ASSERT_TRUE(paint(display, kLoadingImage));
    display.clear();

    EXPECT_EQ(metrics.get("epd_refresh_total"), 3);
    EXPECT_EQ(metrics.get("epd_image_refresh_total"), 2);
    EXPECT_EQ(metrics.get("epd_clear_total"), 1);
    EXPECT_GT(metrics.get("epd_first_use_at"), 0);
}

TEST_F(MockPanelTest, UndecodableImageLeavesThePanelUntouched)
{
    const auto bogus = db.dir() / "bogus.jpg";
    std::ofstream(bogus) << "not an image";
    DisplayManager display(cfg, metrics);
    EXPECT_TRUE(PhotoRenderer::render(bogus).empty());
    EXPECT_FALSE(paint(display, bogus));
    EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(cfg.display.mockOutputDir) / "panel.json"));
}

TEST_F(MockPanelTest, SameImageGivesIdenticalFrame)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));
    const auto first = read("panel.pbm");
    ASSERT_TRUE(paint(display, kLoadingImage));
    EXPECT_EQ(read("panel.pbm"), first);
}

TEST_F(MockPanelTest, ImmediateRefreshIgnoresMinInterval)
{
    cfg.display.minRefreshSecs = 3600;
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));

    ASSERT_TRUE(display.displayBuffer(PhotoRenderer::render(kLoadingImage), "urgent", {}, RefreshPolicy::Immediate));
    EXPECT_EQ(meta()["source"], "urgent");
    EXPECT_EQ(meta()["seq"], 2);
}

TEST_F(MockPanelTest, StopCancelsRefreshWaitingForMinInterval)
{
    cfg.display.minRefreshSecs = 3600;
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));

    std::atomic<bool> done{false};
    std::atomic<bool> painted{true};
    std::jthread pending([&](const std::stop_token& st)
    {
        painted = display.displayBuffer(PhotoRenderer::render(kLoadingImage), "late", st);
        done = true;
    });
    pending.request_stop();

    ASSERT_TRUE(waitFor([&] { return done.load(); }));
    EXPECT_FALSE(painted);
    EXPECT_EQ(meta()["seq"], 1);
}

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

    ASSERT_TRUE(display.displayBuffer(PhotoRenderer::render(kLoadingImage), "urgent", {}, RefreshPolicy::Immediate,
                                      {"Rahmen einrichten", "Code 482 913"}));
    EXPECT_EQ(meta()["source"], "urgent");
    EXPECT_EQ(meta()["seq"], 2);
    EXPECT_EQ(meta()["text"], nlohmann::json::array({"Rahmen einrichten", "Code 482 913"})) << "the specs read codes from here";
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

namespace
{
std::vector<uint8_t> withBlackCorner(std::vector<uint8_t> buffer)
{
    for (int y = 0; y < 16; ++y)
        buffer[static_cast<size_t>(y) * (DisplayManager::WIDTH / 8)] = 0x00;
    return buffer;
}
} // namespace

TEST_F(MockPanelTest, ASmallChangeIsAPartialRefreshOfTheWholeFrame)
{
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayBuffer(photo, "photo"));
    const auto changed = withBlackCorner(photo);

    ASSERT_TRUE(display.displayChanges(changed, "photo+icon"));

    EXPECT_EQ(meta()["refresh"], "partial");
    EXPECT_EQ(meta()["source"], "photo+icon");
    EXPECT_EQ(meta()["seq"], 2);
    std::string expected;
    for (const uint8_t b : changed)
        expected.push_back(static_cast<char>(~b));
    EXPECT_EQ(read("panel.pbm").substr(kHeader.size()), expected);
    EXPECT_EQ(metrics.get("epd_partial_refresh_total"), 1);
    EXPECT_EQ(metrics.get("epd_refresh_total"), 1) << "partials don't count as wear of the whole panel";
}

TEST_F(MockPanelTest, WhatThePanelAlreadyShowsIsNotRefreshedAgain)
{
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayBuffer(photo, "photo"));

    ASSERT_TRUE(display.displayChanges(photo, "photo"));

    EXPECT_EQ(meta()["seq"], 1);
    EXPECT_EQ(display.screenSnapshot().seq, 1);
}

TEST_F(MockPanelTest, AnUnknownPanelOrABigChangeGetsAFullRefresh)
{
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayChanges(photo, "first"));
    EXPECT_EQ(meta()["refresh"], "full");

    ASSERT_TRUE(display.displayChanges(std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0x00), "black page"));
    EXPECT_EQ(meta()["refresh"], "full");
    EXPECT_EQ(metrics.get("epd_partial_refresh_total"), 0);
}

TEST_F(MockPanelTest, AFullRefreshClearsTheGhostingOfPartialsInARow)
{
    cfg.display.maxPartialRefreshes = 2;
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayBuffer(photo, "photo"));

    ASSERT_TRUE(display.displayChanges(withBlackCorner(photo), "on"));
    ASSERT_TRUE(display.displayChanges(photo, "off"));
    ASSERT_TRUE(display.displayChanges(withBlackCorner(photo), "on again"));

    EXPECT_EQ(meta()["refresh"], "full");
    EXPECT_EQ(metrics.get("epd_partial_refresh_total"), 2);
}

TEST_F(MockPanelTest, SwitchedOffPartialRefreshesPaintInFull)
{
    cfg.display.partialRefresh = false;
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayBuffer(photo, "photo"));

    ASSERT_TRUE(display.displayChanges(withBlackCorner(photo), "photo+icon"));

    EXPECT_EQ(meta()["refresh"], "full");
}

TEST_F(MockPanelTest, APartialRefreshWaitsForTheMinIntervalLikeAnyOther)
{
    cfg.display.minRefreshSecs = 3600;
    DisplayManager display(cfg, metrics);
    const auto photo = PhotoRenderer::render(kLoadingImage);
    ASSERT_TRUE(display.displayBuffer(photo, "photo"));

    std::atomic<bool> done{false};
    std::atomic<bool> painted{true};
    std::jthread pending([&](const std::stop_token& st)
    {
        painted = display.displayChanges(withBlackCorner(photo), "photo+icon", st);
        done = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(done.load());
    pending.request_stop();

    ASSERT_TRUE(waitFor([&] { return done.load(); }));
    EXPECT_FALSE(painted);
    EXPECT_EQ(meta()["seq"], 1);
}

TEST_F(MockPanelTest, AfterAClearASmallDrawingIsAPartialRefresh)
{
    DisplayManager display(cfg, metrics);
    ASSERT_TRUE(paint(display, kLoadingImage));
    display.clear();

    ASSERT_TRUE(display.displayChanges(withBlackCorner(std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF)), "note"));

    EXPECT_EQ(meta()["refresh"], "partial");
    EXPECT_EQ(meta()["state"], "image");
}

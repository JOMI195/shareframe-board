#include "display/LogoScreenRenderer.hpp"
#include "display/PanelText.hpp"
#include "display/PhotoRenderer.hpp"
#include <gtest/gtest.h>

namespace
{
const std::string kAssets = SHAREFRAME_ASSETS_DIR;
const std::string kImage = kAssets + "/image/loading/" + LogoScreenRenderer::IMAGE;

std::vector<uint8_t> render(const std::string& image, const std::string& detail)
{
    return LogoScreenRenderer::render(image, "Wird geladen …", detail, kAssets + "/font/DejaVuSans-Bold.ttf",
                                      kAssets + "/font/DejaVuSans.ttf");
}

bool isBlack(const std::vector<uint8_t>& buffer, const int x, const int y)
{
    const int i = y * DisplayManager::WIDTH + x;
    return !(buffer[i / 8] & (0x80 >> (i % 8)));
}

struct Ink
{
    int count = 0, x0 = DisplayManager::WIDTH, y0 = DisplayManager::HEIGHT, x1 = -1, y1 = -1;
};

/// Pixels black in `buffer` but not in `base`.
Ink added(const std::vector<uint8_t>& buffer, const std::vector<uint8_t>& base)
{
    Ink ink;
    for (int y = 0; y < DisplayManager::HEIGHT; ++y)
        for (int x = 0; x < DisplayManager::WIDTH; ++x)
            if (isBlack(buffer, x, y) && !isBlack(base, x, y))
            {
                ++ink.count;
                ink.x0 = std::min(ink.x0, x);
                ink.y0 = std::min(ink.y0, y);
                ink.x1 = std::max(ink.x1, x);
                ink.y1 = std::max(ink.y1, y);
            }
    return ink;
}
}

TEST(LogoScreenRenderer, TextSitsCentredBelowTheLogoInsideTheVisibleArea)
{
    const auto ink = added(render(kImage, "v1.2.0"), PhotoRenderer::render(kImage));
    ASSERT_GT(ink.count, 0);

    constexpr auto safe = panel_text::SAFE_AREA;
    EXPECT_GE(ink.x0, safe.x0);
    EXPECT_LT(ink.x1, safe.x1);
    EXPECT_GE(ink.y0, 300) << "text runs into the logo";
    EXPECT_LE(ink.y1, 420);
    EXPECT_LE(ink.y1, safe.y1);
    EXPECT_NEAR((ink.x0 + ink.x1) / 2.0, DisplayManager::WIDTH / 2.0, 3);
}

TEST(LogoScreenRenderer, EmptyDetailDrawsNoSecondLine)
{
    const auto ink = added(render(kImage, ""), PhotoRenderer::render(kImage));
    ASSERT_GT(ink.count, 0);
    EXPECT_LT(ink.y1, 375);
}

TEST(LogoScreenRenderer, MissingPictureStillShowsTheText)
{
    const auto buffer = render(kAssets + "/image/loading/missing.jpg", "v1.2.0");
    ASSERT_EQ(buffer.size(), DisplayManager::BUFFER_SIZE);
    EXPECT_GT(added(buffer, std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF)).count, 0);
}

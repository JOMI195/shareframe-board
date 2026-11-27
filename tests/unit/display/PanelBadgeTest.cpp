#include "display/PanelBadge.hpp"
#include <gtest/gtest.h>

TEST(PanelBadge, NightTextSaysWhenPicturesChangeAgain)
{
    EXPECT_EQ(panel_badge::nightText(7), "Nachtmodus bis 7 Uhr");
}

namespace
{
bool inside(const panel_text::Rect& r, const panel_text::Rect& outer)
{
    return r.x0 >= outer.x0 && r.y0 >= outer.y0 && r.x1 <= outer.x1 && r.y1 <= outer.y1;
}

bool overlaps(const panel_text::Rect& a, const panel_text::Rect& b)
{
    return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}
}

TEST(PanelBadge, RandomPlacementStaysVisibleAndClearOfTheLastSpot)
{
    std::mt19937 rng(42);
    std::vector<panel_text::Rect> avoid;
    for (int i = 0; i < 200; ++i)
    {
        const auto r = panel_badge::randomPlacement(400, 44, rng, avoid);
        EXPECT_TRUE(inside(r, panel_text::SAFE_AREA));
        EXPECT_EQ(r.x1 - r.x0, 400);
        EXPECT_EQ(r.y1 - r.y0, 44);
        for (const auto& a : avoid)
            EXPECT_FALSE(overlaps(r, a));
        avoid = {r, panel_badge::area(panel_badge::Corner::BottomRight)};
    }
}

TEST(PanelBadge, RandomPlacementShrinksBoxesLargerThanTheVisibleArea)
{
    std::mt19937 rng(1);
    const auto r = panel_badge::randomPlacement(5000, 5000, rng, {});
    EXPECT_TRUE(inside(r, panel_text::SAFE_AREA));
}

namespace
{
bool isBlack(const std::vector<uint8_t>& buffer, const int x, const int y)
{
    const int i = y * DisplayManager::WIDTH + x;
    return !(buffer[i / 8] & (0x80 >> (i % 8)));
}
}

TEST(PanelBadge, TheWifiOffSignStaysInItsBoxInTheVisibleArea)
{
    constexpr auto box = panel_badge::wifiOffArea();
    EXPECT_TRUE(inside(box, panel_text::SAFE_AREA));
    for (const auto corner : {panel_badge::Corner::TopRight, panel_badge::Corner::BottomRight})
        EXPECT_FALSE(overlaps(box, panel_badge::area(corner)));

    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0x00);
    panel_badge::drawWifiOff(buffer);
    int changed = 0;
    for (int y = 0; y < DisplayManager::HEIGHT; ++y)
        for (int x = 0; x < DisplayManager::WIDTH; ++x)
            if (!isBlack(buffer, x, y))
            {
                ++changed;
                EXPECT_TRUE(inside({x, y, x + 1, y + 1}, box)) << x << "," << y;
            }
    EXPECT_GT(changed, 0) << "the box is white behind the sign";
}

TEST(PanelBadge, TheWifiOffSignIsABoxedSignStruckThrough)
{
    constexpr auto box = panel_badge::wifiOffArea();
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    panel_badge::drawWifiOff(buffer);

    EXPECT_TRUE(isBlack(buffer, box.x0, box.y0)) << "border";
    EXPECT_TRUE(isBlack(buffer, box.x1 - 1, box.y1 - 1)) << "border";
    EXPECT_TRUE(isBlack(buffer, box.x0 + 22, box.y0 + 33)) << "dot";
    EXPECT_TRUE(isBlack(buffer, box.x0 + 22, box.y0 + 12)) << "outer arc";
    EXPECT_TRUE(isBlack(buffer, box.x0 + 14, box.y0 + 14)) << "strike";
    EXPECT_TRUE(isBlack(buffer, box.x0 + 33, box.y0 + 33)) << "strike";
    EXPECT_FALSE(isBlack(buffer, box.x0 + 5, box.y0 + 38)) << "inside the box";
}

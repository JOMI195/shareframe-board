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

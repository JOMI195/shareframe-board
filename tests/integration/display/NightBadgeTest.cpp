#include "display/NightBadge.hpp"
#include <gtest/gtest.h>

namespace
{
const std::string kFont = std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans-Bold.ttf";

bool isBlack(const std::vector<uint8_t>& buffer, const int x, const int y)
{
    const int i = y * DisplayManager::WIDTH + x;
    return !(buffer[i / 8] & (0x80 >> (i % 8)));
}

int count(const std::vector<uint8_t>& buffer, const bool black, const panel_text::Rect& r)
{
    int n = 0;
    for (int y = r.y0; y < r.y1; ++y)
        for (int x = r.x0; x < r.x1; ++x)
            n += isBlack(buffer, x, y) == black;
    return n;
}
}

TEST(NightBadgeDraw, InksOnlyTheBadgeCorner)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    night_badge::draw(buffer, kFont, night_badge::text(23));

    const int inside = count(buffer, true, night_badge::BADGE_AREA);
    EXPECT_GT(inside, 0);
    EXPECT_EQ(count(buffer, true, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}), inside);
    const auto& a = night_badge::BADGE_AREA;
    EXPECT_EQ(count(buffer, true, {a.x0, a.y0, a.x0 + 1, a.y1}), 0) << "text wider than the badge area";
}

TEST(NightBadgeDraw, StaysReadableOnADarkPhoto)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0x00);
    night_badge::draw(buffer, kFont, night_badge::text(23));

    EXPECT_GT(count(buffer, false, night_badge::BADGE_AREA), 0);
    EXPECT_EQ(count(buffer, false, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}),
              count(buffer, false, night_badge::BADGE_AREA));
}

TEST(NightBadgeDraw, MissingFontLeavesThePhotoAlone)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    night_badge::draw(buffer, "/nonexistent.ttf", night_badge::text(23));
    EXPECT_EQ(buffer, std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF));
}

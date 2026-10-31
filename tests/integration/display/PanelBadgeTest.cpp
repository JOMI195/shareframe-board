#include "display/PanelBadge.hpp"
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

using panel_badge::Corner;

TEST(PanelBadgeDraw, InksOnlyItsCorner)
{
    for (const auto corner : {Corner::TopRight, Corner::BottomRight})
    {
        std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
        panel_badge::draw(buffer, kFont, panel_badge::nightText(23), corner);

        const auto a = panel_badge::area(corner);
        const int inside = count(buffer, true, a);
        EXPECT_GT(inside, 0);
        EXPECT_EQ(count(buffer, true, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}), inside);
        EXPECT_EQ(count(buffer, true, {a.x0, a.y0, a.x0 + 1, a.y1}), 0) << "text wider than the badge area";
    }
}

TEST(PanelBadgeDraw, LongNameIsCutShortInsideItsBox)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    panel_badge::draw(buffer, kFont, std::string(150, 'W'), Corner::TopRight);

    // Text running past the box would ink the padding just inside its right border.
    const auto a = panel_badge::area(Corner::TopRight);
    EXPECT_EQ(count(buffer, true, {a.x1 - 8, a.y0 + 10, a.x1 - 3, a.y0 + 25}), 0);
    EXPECT_GT(count(buffer, true, {a.x0 + 20, a.y0 + 10, a.x1 - 20, a.y0 + 25}), 0);
}

TEST(PanelBadgeDraw, StaysReadableOnADarkPhoto)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0x00);
    panel_badge::draw(buffer, kFont, panel_badge::nightText(23), Corner::BottomRight);

    const auto a = panel_badge::area(Corner::BottomRight);
    EXPECT_GT(count(buffer, false, a), 0);
    EXPECT_EQ(count(buffer, false, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}), count(buffer, false, a));
}

TEST(PanelBadgeDraw, MissingFontLeavesThePhotoAlone)
{
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    panel_badge::draw(buffer, "/nonexistent.ttf", "alice", Corner::TopRight);
    EXPECT_EQ(buffer, std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF));
}

TEST(PanelBadgeDraw, AnywhereInksOnlyTheReturnedBox)
{
    std::mt19937 rng(7);
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    const auto box = panel_badge::drawAnywhere(buffer, kFont, std::string(panel_badge::NO_IMAGES_TEXT), rng, {});

    ASSERT_TRUE(box.has_value());
    const int inked = count(buffer, true, *box);
    EXPECT_GT(inked, 0);
    EXPECT_EQ(count(buffer, true, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}), inked);
}

TEST(PanelBadgeDraw, AnywhereLeavesTheBufferWithoutAFont)
{
    std::mt19937 rng(7);
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0xFF);
    EXPECT_FALSE(panel_badge::drawAnywhere(buffer, "/nonexistent.ttf", "x", rng, {}).has_value());
    EXPECT_EQ(buffer, std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF));
}

#include "display/DisplayManager.hpp"
#include "display/PartialRefresh.hpp"
#include <gtest/gtest.h>

using namespace partial_refresh;

namespace
{
std::vector<uint8_t> white()
{
    return std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF);
}

void blacken(std::vector<uint8_t>& buffer, const int x, const int y)
{
    buffer[static_cast<size_t>(y) * (DisplayManager::WIDTH / 8) + x / 8] &= static_cast<uint8_t>(~(0x80 >> (x % 8)));
}
} // namespace

TEST(PartialRefresh, MatchingBuffersChangeNothing)
{
    EXPECT_EQ(changedWindow(white(), white()), std::nullopt);
}

TEST(PartialRefresh, TheWindowSpansEveryChangedPixelOnWholeBytes)
{
    auto next = white();
    blacken(next, 13, 40);
    blacken(next, 70, 2);
    EXPECT_EQ(changedWindow(white(), next), (Window{8, 2, 72, 41}));
}

TEST(PartialRefresh, ThePanelsLastByteAndRowAreReachable)
{
    auto next = white();
    blacken(next, DisplayManager::WIDTH - 1, DisplayManager::HEIGHT - 1);
    EXPECT_EQ(changedWindow(white(), next),
              (Window{DisplayManager::WIDTH - 8, DisplayManager::HEIGHT - 1, DisplayManager::WIDTH, DisplayManager::HEIGHT}));
}

TEST(PartialRefresh, ABufferOfAnotherSizeChangesTheWholePanel)
{
    EXPECT_EQ(changedWindow({}, white()), (Window{0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}));
}

TEST(PartialRefresh, ASmallChangeRefreshesOnlyItsWindow)
{
    EXPECT_EQ(choose(white(), Window{0, 0, 48, 48}, 0, {}), Kind::Partial);
}

TEST(PartialRefresh, AnUnknownPanelGetsAFullRefresh)
{
    EXPECT_EQ(choose({}, Window{0, 0, 48, 48}, 0, {}), Kind::Full);
    EXPECT_EQ(choose({}, std::nullopt, 0, {}), Kind::Full);
}

TEST(PartialRefresh, NoChangeNeedsNoRefresh)
{
    EXPECT_EQ(choose(white(), std::nullopt, 0, {}), Kind::None);
}

TEST(PartialRefresh, ALargeChangeRefreshesTheWholePanel)
{
    const Limits limits{.maxAreaPercent = 25};
    EXPECT_EQ(choose(white(), Window{0, 0, 400, 240}, 0, limits), Kind::Partial);
    EXPECT_EQ(choose(white(), Window{0, 0, 408, 240}, 0, limits), Kind::Full);
}

TEST(PartialRefresh, AFullRefreshFollowsTooManyPartialsInARow)
{
    const Limits limits{.maxInARow = 5};
    EXPECT_EQ(choose(white(), Window{0, 0, 48, 48}, 4, limits), Kind::Partial);
    EXPECT_EQ(choose(white(), Window{0, 0, 48, 48}, 5, limits), Kind::Full);
}

TEST(PartialRefresh, SwitchedOffEveryChangeRefreshesTheWholePanel)
{
    EXPECT_EQ(choose(white(), Window{0, 0, 48, 48}, 0, {.enabled = false}), Kind::Full);
}

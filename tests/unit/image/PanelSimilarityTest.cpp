#include "display/DisplayManager.hpp"
#include "image/PanelSimilarity.hpp"
#include <gtest/gtest.h>

// Error diffusion scatters the dots of one photo differently after the slightest change, so a per-pixel
// comparison would call two copies of it different.
TEST(PanelSimilarity, TwoDitherPatternsOfTheSameGreyCountAsTheSame)
{
    const std::vector<uint8_t> checker(DisplayManager::BUFFER_SIZE, 0b10101010);
    const std::vector<uint8_t> shifted(DisplayManager::BUFFER_SIZE, 0b01010101);
    const std::vector<uint8_t> white(DisplayManager::BUFFER_SIZE, 0xFF);
    const std::vector<uint8_t> black(DisplayManager::BUFFER_SIZE, 0x00);

    EXPECT_DOUBLE_EQ(panel_similarity::difference(checker, shifted), 0.0);
    EXPECT_DOUBLE_EQ(panel_similarity::difference(checker, white), 0.5);
    EXPECT_DOUBLE_EQ(panel_similarity::difference(white, black), 1.0);
}

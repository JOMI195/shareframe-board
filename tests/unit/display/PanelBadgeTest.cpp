#include "display/PanelBadge.hpp"
#include <gtest/gtest.h>

TEST(PanelBadge, NightTextSaysWhenPicturesChangeAgain)
{
    EXPECT_EQ(panel_badge::nightText(7), "Nachtmodus bis 7 Uhr");
}

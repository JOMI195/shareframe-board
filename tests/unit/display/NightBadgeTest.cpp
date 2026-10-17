#include "display/NightBadge.hpp"
#include <gtest/gtest.h>

TEST(NightBadge, SaysWhenPicturesChangeAgain)
{
    EXPECT_EQ(night_badge::text(7), "Nachtmodus bis 7 Uhr");
}

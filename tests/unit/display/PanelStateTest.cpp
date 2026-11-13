#include "display/PanelState.hpp"
#include <gtest/gtest.h>

TEST(PanelState, PowerIsOnlyCutOnASleepingPanel)
{
    EXPECT_TRUE(canMove(PanelPower::Off, PanelPower::Awake));
    EXPECT_TRUE(canMove(PanelPower::Awake, PanelPower::Asleep));
    EXPECT_TRUE(canMove(PanelPower::Asleep, PanelPower::Awake));
    EXPECT_TRUE(canMove(PanelPower::Asleep, PanelPower::Off));

    EXPECT_FALSE(canMove(PanelPower::Awake, PanelPower::Off));
    EXPECT_FALSE(canMove(PanelPower::Off, PanelPower::Asleep));
}

TEST(PanelState, HealthFollowsTheStreakOfFailedRefreshes)
{
    EXPECT_EQ(panelHealth(0), PanelHealth::Ok);
    EXPECT_EQ(panelHealth(1), PanelHealth::Degraded);
    EXPECT_EQ(panelHealth(2), PanelHealth::Degraded);
    EXPECT_EQ(panelHealth(3), PanelHealth::Failed);
}

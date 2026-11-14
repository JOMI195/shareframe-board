#include "display/StatusWindow.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr int64_t kTimeout = 2400;
}

TEST(StatusWindow, ForgetsAScreenNobodyHidInTime)
{
    StatusWindow window;
    EXPECT_EQ(window.endsAt(kTimeout), std::nullopt);

    window.request({StatusScreenKind::Reboot, ""}, 100);
    EXPECT_EQ(window.endsAt(kTimeout), 100 + kTimeout);
    EXPECT_EQ(window.expire(100 + kTimeout - 1, kTimeout), std::nullopt);
    EXPECT_EQ(window.requested().kind, StatusScreenKind::Reboot);

    EXPECT_EQ(window.expire(100 + kTimeout, kTimeout), StatusScreenKind::Reboot);
    EXPECT_EQ(window.requested(), StatusScreenEvent{});
    EXPECT_EQ(window.expire(100 + 2 * kTimeout, kTimeout), std::nullopt) << "nothing left to forget";
}

TEST(StatusWindow, ANewRequestStartsTheTimeoutAgain)
{
    StatusWindow window;
    window.request({StatusScreenKind::Update, "1.3.0"}, 0);
    window.request({StatusScreenKind::Reboot, ""}, 2000);
    EXPECT_EQ(window.expire(kTimeout, kTimeout), std::nullopt);
    EXPECT_EQ(window.endsAt(kTimeout), 2000 + kTimeout);
}

TEST(StatusWindow, HidingLeavesNothingToTimeOut)
{
    StatusWindow window;
    window.request({StatusScreenKind::Update, "1.3.0"}, 0);
    window.request({}, 10);
    EXPECT_EQ(window.endsAt(kTimeout), std::nullopt);
    EXPECT_EQ(window.expire(10 + kTimeout, kTimeout), std::nullopt);
}

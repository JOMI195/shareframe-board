#include "dashboard/FactoryResetGuard.hpp"
#include <gtest/gtest.h>

namespace
{
using Issue = FactoryResetGuard::IssueResult;
using Check = FactoryResetGuard::CheckResult;

struct Guard
{
    FactoryResetGuard::Clock::time_point now = FactoryResetGuard::Clock::now();
    int codes = 0;
    FactoryResetGuard guard{[this] { return now; }, [this] { return std::to_string(100000 + ++codes); }};
};
}

TEST(FactoryResetGuard, TheRightCodeWorksOnce)
{
    Guard g;
    const auto issued = g.guard.issue();
    ASSERT_EQ(issued.result, Issue::Fresh);
    EXPECT_EQ(g.guard.verify(issued.code).result, Check::Accepted);
    EXPECT_EQ(g.guard.verify(issued.code).result, Check::NoCode);
}

TEST(FactoryResetGuard, FiveWrongTriesKillTheCode)
{
    Guard g;
    const auto code = g.guard.issue().code;
    for (int i = FactoryResetGuard::kMaxTries - 1; i > 0; --i)
        EXPECT_EQ(g.guard.verify("000000").triesLeft, i);
    EXPECT_EQ(g.guard.verify("000000").result, Check::Wrong);
    EXPECT_EQ(g.guard.verify(code).result, Check::NoCode);
}

TEST(FactoryResetGuard, ACodeEndsAfterFiveMinutes)
{
    Guard g;
    const auto code = g.guard.issue().code;
    g.now += FactoryResetGuard::kValidity;
    EXPECT_EQ(g.guard.verify(code).result, Check::NoCode);
}

// Asking again while a code is up must not repaint the panel with a new one.
TEST(FactoryResetGuard, ASecondRequestKeepsThePendingCode)
{
    Guard g;
    const auto first = g.guard.issue();
    const auto again = g.guard.issue();
    EXPECT_EQ(again.result, Issue::Pending);
    EXPECT_TRUE(again.code.empty());
    EXPECT_EQ(g.guard.verify(first.code).result, Check::Accepted);
}

TEST(FactoryResetGuard, ThreeCodesInARowWithoutAResetLockForAnHour)
{
    Guard g;
    (void)g.guard.issue();
    g.guard.cancel();
    (void)g.guard.issue();
    g.now += FactoryResetGuard::kValidity;
    (void)g.guard.issue();
    for (int i = 0; i < FactoryResetGuard::kMaxTries; ++i)
        (void)g.guard.verify("000000");

    const auto locked = g.guard.issue();
    EXPECT_EQ(locked.result, Issue::Locked);
    EXPECT_GT(locked.secondsLeft, 3500);
    g.now += FactoryResetGuard::kLock;
    EXPECT_EQ(g.guard.issue().result, Issue::Fresh);
}

TEST(FactoryResetGuard, AResetClearsTheBurnedCount)
{
    Guard g;
    (void)g.guard.issue();
    g.guard.cancel();
    (void)g.guard.issue();
    g.guard.cancel();
    EXPECT_EQ(g.guard.verify(g.guard.issue().code).result, Check::Accepted);
    (void)g.guard.issue();
    g.guard.cancel();
    EXPECT_EQ(g.guard.issue().result, Issue::Fresh);
}

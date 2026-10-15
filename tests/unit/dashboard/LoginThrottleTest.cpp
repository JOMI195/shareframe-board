#include "dashboard/LoginThrottle.hpp"
#include <gtest/gtest.h>

TEST(LoginThrottle, LocksAfterFiveFailures)
{
    LoginThrottle t;
    for (int i = 0; i < 4; ++i)
        t.recordFailure();
    EXPECT_EQ(t.retryAfterSecs(), 0);

    t.recordFailure();
    EXPECT_GT(t.retryAfterSecs(), 0);
    EXPECT_LE(t.retryAfterSecs(), 30);
}

TEST(LoginThrottle, SuccessResetsFailuresAndLock)
{
    LoginThrottle t;
    for (int i = 0; i < 5; ++i)
        t.recordFailure();
    t.recordSuccess();
    EXPECT_EQ(t.retryAfterSecs(), 0);

    for (int i = 0; i < 4; ++i)
        t.recordFailure();
    EXPECT_EQ(t.retryAfterSecs(), 0);
}

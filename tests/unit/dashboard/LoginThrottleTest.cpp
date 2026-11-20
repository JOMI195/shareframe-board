#include "dashboard/LoginThrottle.hpp"
#include <gtest/gtest.h>

namespace
{
struct ThrottleClock
{
    LoginThrottle::Clock::time_point now = LoginThrottle::Clock::now();
    LoginThrottle throttle{[this] { return now; }};

    bool fail(const std::string& client)
    {
        return throttle.begin(client).admitted();
    }
};
}

TEST(LoginThrottle, FiveFreeFailuresThenALockThatDoublesUpToAnHour)
{
    ThrottleClock t;
    for (int i = 0; i < LoginThrottle::kFreeFailures; ++i)
        EXPECT_TRUE(t.fail("phone")) << i;
    EXPECT_EQ(t.throttle.begin("phone").retryAfterSecs(), 30);

    t.now += std::chrono::seconds(30);
    EXPECT_TRUE(t.fail("phone"));
    EXPECT_EQ(t.throttle.begin("phone").retryAfterSecs(), 60);

    for (int i = 0; i < 12; ++i)
    {
        t.now += LoginThrottle::kMaxLock;
        t.fail("phone");
    }
    EXPECT_EQ(t.throttle.begin("phone").retryAfterSecs(), 3600);
}

TEST(LoginThrottle, OneClientsGuessesDoNotLockAnother)
{
    ThrottleClock t;
    for (int i = 0; i < 10; ++i)
        t.fail("neighbour");
    EXPECT_TRUE(t.throttle.begin("owner").admitted());
}

TEST(LoginThrottle, ASuccessClearsTheClientsFailures)
{
    ThrottleClock t;
    for (int i = 0; i < LoginThrottle::kFreeFailures - 1; ++i)
        t.fail("owner");
    t.throttle.begin("owner").succeeded();
    for (int i = 0; i < LoginThrottle::kFreeFailures; ++i)
        EXPECT_TRUE(t.fail("owner")) << i;
}

TEST(LoginThrottle, OnlyOneVerificationRunsAtATime)
{
    ThrottleClock t;
    const auto running = t.throttle.begin("a");
    ASSERT_TRUE(running.admitted());
    const auto parallel = t.throttle.begin("b");
    EXPECT_FALSE(parallel.admitted());
    EXPECT_GT(parallel.retryAfterSecs(), 0);
}

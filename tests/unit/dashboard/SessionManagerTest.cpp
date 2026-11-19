#include "dashboard/SessionManager.hpp"
#include <gtest/gtest.h>

TEST(SessionManager, CreatedSessionIsValidUntilRemoved)
{
    SessionManager sm;
    const auto id = sm.createSession();
    EXPECT_EQ(id.size(), 64u);
    EXPECT_TRUE(sm.isValid(id));

    sm.removeSession(id);
    EXPECT_FALSE(sm.isValid(id));
}

TEST(SessionManager, SessionsAreUniqueAndUnknownIdsInvalid)
{
    SessionManager sm;
    EXPECT_NE(sm.createSession(), sm.createSession());
    EXPECT_FALSE(sm.isValid(""));
    EXPECT_FALSE(sm.isValid(std::string(64, '0')));
}

TEST(SessionManager, ASessionInUseStaysOpen)
{
    using std::chrono::minutes;
    auto now = SessionManager::Clock::now();
    SessionManager sm{[&] { return now; }};
    const auto id = sm.createSession();

    for (int i = 0; i < 6; ++i)
    {
        now += minutes(20);
        EXPECT_TRUE(sm.isValid(id)) << "after " << (i + 1) * 20 << " min in use";
    }
}

TEST(SessionManager, ASessionLeftAloneEndsAfterTheIdleTime)
{
    auto now = SessionManager::Clock::now();
    SessionManager sm{[&] { return now; }};
    const auto id = sm.createSession();

    now += SessionManager::kSessionIdle - std::chrono::seconds(1);
    EXPECT_TRUE(sm.isValid(id));
    now += SessionManager::kSessionIdle;
    EXPECT_FALSE(sm.isValid(id));
}

TEST(SessionManager, ExpiredSessionsAreDroppedWhenANewOneStarts)
{
    auto now = SessionManager::Clock::now();
    SessionManager sm{[&] { return now; }};
    sm.createSession();
    sm.createSession();
    now += SessionManager::kSessionIdle + std::chrono::seconds(1);
    sm.createSession();
    EXPECT_EQ(sm.count(), 1u);
}

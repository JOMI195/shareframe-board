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

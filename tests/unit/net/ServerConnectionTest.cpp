#include "net/ServerConnection.hpp"
#include <gtest/gtest.h>

TEST(ServerConnection, RoundTripsThroughJson)
{
    const ServerConnection c{false, 1700, 1800};
    EXPECT_EQ(serverConnectionFromJson(toJson(c)), c);
    EXPECT_EQ(toJson(c)["last_contact_at"], 1800);
}

TEST(ServerConnection, ForeignRepliesAreNotMistakenForAState)
{
    EXPECT_FALSE(serverConnectionFromJson(nlohmann::json::object()));
    EXPECT_FALSE(serverConnectionFromJson({{"running", true}}));
    EXPECT_FALSE(serverConnectionFromJson({{"connected", "yes"}}));
    EXPECT_FALSE(serverConnectionFromJson(nullptr));
}

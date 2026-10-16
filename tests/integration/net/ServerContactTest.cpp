#include "net/ServerContact.hpp"
#include "support/TestDb.hpp"
#include <gtest/gtest.h>

namespace
{
class ServerContactTest : public ::testing::Test
{
protected:
    ServerContact make() { return ServerContact{repo, [this] { return now; }}; }

    TestDb db;
    SettingsRepository repo{db.get()};
    int64_t now = 1'000'000;
};
}

TEST_F(ServerContactTest, NeverConnectedReportsNoContact)
{
    EXPECT_EQ(make().snapshot(), (ServerConnection{false, 0, 0}));
}

TEST_F(ServerContactTest, OnlineSinceOpenOfflineSinceClose)
{
    auto contact = make();
    contact.onOpen();
    now += 100;
    contact.touch();
    EXPECT_EQ(contact.snapshot(), (ServerConnection{true, 1'000'000, 1'000'100}));

    now += 50;
    contact.onClose();
    now += 50;
    contact.onClose();
    EXPECT_EQ(contact.snapshot(), (ServerConnection{false, 1'000'150, 1'000'150}));
}

TEST_F(ServerContactTest, ARestartRemembersTheLastContact)
{
    {
        auto contact = make();
        contact.onOpen();
        now += 30;
        contact.onClose();
    }
    now += 10'000;
    EXPECT_EQ(make().snapshot(), (ServerConnection{false, 1'000'030, 1'000'030}));
}

TEST_F(ServerContactTest, TouchesOnlyReachTheDiskOncePerPeriod)
{
    auto contact = make();
    contact.onOpen();
    now += ServerContact::PERSIST_EVERY_SECS - 1;
    contact.touch();
    EXPECT_EQ(repo.get("server_last_contact_at"), "1000000");

    now += 1;
    contact.touch();
    EXPECT_EQ(repo.get("server_last_contact_at"), std::to_string(now));
}

TEST_F(ServerContactTest, TouchWhileOfflineChangesNothing)
{
    auto contact = make();
    contact.touch();
    EXPECT_EQ(contact.snapshot(), (ServerConnection{false, 0, 0}));
    EXPECT_FALSE(repo.get("server_last_contact_at"));
}

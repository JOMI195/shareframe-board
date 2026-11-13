#include "net/ServerLink.hpp"
#include <array>
#include <gtest/gtest.h>
#include <set>
#include <utility>

using ws::Link;

namespace
{
constexpr std::array kAll = {Link::Connecting, Link::Unregistered, Link::Online, Link::Offline};
}

TEST(ServerLink, OnlyTheListedMovesAreAllowed)
{
    const std::set<std::pair<Link, Link>> allowed = {
        {Link::Connecting, Link::Unregistered}, {Link::Connecting, Link::Online},
        {Link::Unregistered, Link::Online}, {Link::Online, Link::Offline}, {Link::Offline, Link::Online},
    };
    for (const auto from : kAll)
    {
        for (const auto to : kAll)
        {
            if (from != to)
                EXPECT_EQ(ws::canMove(from, to), allowed.contains({from, to})) << name(from) << " -> " << name(to);
        }
    }
}

TEST(ServerLink, EveryReactionStaysWithinTheRules)
{
    for (const auto link : kAll)
    {
        for (const auto next : {ws::onOpened(link), ws::onClosed(link), ws::onRefusedAsUnowned(link)})
            EXPECT_TRUE(next == link || ws::canMove(link, next)) << name(link) << " -> " << name(next);
    }
}

TEST(ServerLink, ARefusalOnlyCountsBeforeTheFirstConnection)
{
    EXPECT_EQ(ws::onRefusedAsUnowned(Link::Connecting), Link::Unregistered);
    EXPECT_EQ(ws::onRefusedAsUnowned(Link::Online), Link::Online);
    EXPECT_EQ(ws::onRefusedAsUnowned(Link::Offline), Link::Offline) << "the setup screen would cover the photos";
}

TEST(ServerLink, ClosingBeforeTheFirstConnectionChangesNothing)
{
    EXPECT_EQ(ws::onClosed(Link::Online), Link::Offline);
    EXPECT_EQ(ws::onClosed(Link::Connecting), Link::Connecting);
    EXPECT_EQ(ws::onClosed(Link::Unregistered), Link::Unregistered) << "the fast retry has to go on";
}

TEST(ServerLink, OnlyAnUnregisteredFrameRetriesOften)
{
    EXPECT_LT(ws::maxReconnectWaitMs(Link::Unregistered), ws::maxReconnectWaitMs(Link::Offline));
    EXPECT_EQ(ws::maxReconnectWaitMs(Link::Connecting), ws::maxReconnectWaitMs(Link::Offline));
}

TEST(ServerLink, ARejectedTokenIsReplacedAtOnceAStaleOneAfterAStreak)
{
    EXPECT_TRUE(ws::refreshesToken(ws::CLOSE_AUTH_REJECTED, 1));
    EXPECT_TRUE(ws::refreshesToken(ws::CLOSE_TOKEN_REVOKED, 1));
    EXPECT_FALSE(ws::refreshesToken(1006, 4));
    EXPECT_TRUE(ws::refreshesToken(1006, 5));
}

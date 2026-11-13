#include "update/UpdatePhase.hpp"
#include <array>
#include <gtest/gtest.h>
#include <set>
#include <utility>

namespace
{
using enum UpdatePhase;
constexpr std::array kAll = {Idle, Checking, Downloading, Installing, AwaitingReboot, Failed};
}

TEST(UpdatePhase, AnUpdateOnlyMovesForwardOrFails)
{
    const std::set<std::pair<UpdatePhase, UpdatePhase>> allowed = {
        {Idle, Checking}, {Failed, Checking},
        {Checking, Downloading}, {Downloading, Installing}, {Installing, AwaitingReboot},
        {Checking, Failed}, {Downloading, Failed}, {Installing, Failed}, {AwaitingReboot, Failed},
    };
    for (const auto from : kAll)
    {
        for (const auto to : kAll)
        {
            if (from != to)
                EXPECT_EQ(canMove(from, to), allowed.contains({from, to})) << name(from) << " -> " << name(to);
        }
    }
}

TEST(UpdatePhase, OnlyRestAndFailureLetTheFrameBeRebootedFromOutside)
{
    EXPECT_FALSE(isBusy(Idle));
    EXPECT_FALSE(isBusy(Failed));
    for (const auto phase : {Checking, Downloading, Installing, AwaitingReboot})
        EXPECT_TRUE(isBusy(phase)) << name(phase);
}

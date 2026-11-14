#include "fsm/Machine.hpp"
#include <gtest/gtest.h>

namespace
{
enum class Door { Open, Closed, Locked };

constexpr bool canMove(const Door from, const Door to)
{
    switch (from)
    {
    case Door::Open: return to == Door::Closed;
    case Door::Closed: return to == Door::Open || to == Door::Locked;
    case Door::Locked: return to == Door::Closed;
    }
    return false;
}

constexpr std::string_view name(const Door door)
{
    switch (door)
    {
    case Door::Open: return "Open";
    case Door::Closed: return "Closed";
    case Door::Locked: return "Locked";
    }
    return "?";
}

fsm::Machine<Door> door(const Door initial)
{
    return fsm::Machine<Door>(initial, spdlog::default_logger());
}
} // namespace

TEST(Machine, FollowsAnAllowedMove)
{
    auto machine = door(Door::Open);
    machine.moveTo(Door::Closed, "pushed");
    machine.moveTo(Door::Locked);
    EXPECT_EQ(machine.state(), Door::Locked);
}

TEST(Machine, RefusesAForbiddenMoveAndStaysWhereItWas)
{
    auto machine = door(Door::Open);
    EXPECT_THROW(machine.moveTo(Door::Locked), fsm::ForbiddenMove);
    EXPECT_EQ(machine.state(), Door::Open);
}

TEST(Machine, MovingToTheCurrentStateIsNotAMove)
{
    auto machine = door(Door::Locked);
    EXPECT_NO_THROW(machine.moveTo(Door::Locked)) << "the rules have no Locked -> Locked";
    EXPECT_EQ(machine.state(), Door::Locked);
}

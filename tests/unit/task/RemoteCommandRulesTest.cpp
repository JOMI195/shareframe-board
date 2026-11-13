#include "task/RemoteCommandRules.hpp"
#include <gtest/gtest.h>

using remote::Command;

// The texts and each single refusal are pinned end to end by RemoteCommandTest.
TEST(RemoteCommandRules, OnlyCommandsThatCutAnUpdateShortWaitForIt)
{
    for (const auto command : {Command::Reboot, Command::UpdateInstall, Command::FactoryReset, Command::Shutdown,
                               Command::EnterSafeMode, Command::ExitSafeMode})
        EXPECT_TRUE(remote::needsIdleUpdate(command, ""));

    EXPECT_TRUE(remote::needsIdleUpdate(Command::RestartService, "update"));
    EXPECT_FALSE(remote::needsIdleUpdate(Command::RestartService, "display"));
    EXPECT_FALSE(remote::needsIdleUpdate(Command::RequestDiagnostics, ""));
}

TEST(RemoteCommandRules, SafeModeDoesNotNeedTheBoardsLeaveToReboot)
{
    EXPECT_FALSE(remote::isRebootClass(Command::EnterSafeMode));
    EXPECT_FALSE(remote::isRebootClass(Command::RestartService));
    EXPECT_TRUE(remote::isRebootClass(Command::UpdateInstall));
    EXPECT_TRUE(remote::isRebootClass(Command::Shutdown));
}

TEST(RemoteCommandRules, TheFirstReasonToRefuseWins)
{
    EXPECT_EQ(remote::refusal(std::nullopt, 7, "x", 200, 100), "unknown command");
    EXPECT_EQ(remote::refusal(Command::Reboot, 0, "", 100, 200), "unknown command") << "a command needs an id";
    EXPECT_EQ(remote::refusal(Command::Reboot, 7, "now", 200, 100), "invalid argument");
    EXPECT_EQ(remote::refusal(Command::RestartService, 7, "sshd", 200, 100), "invalid argument");
    EXPECT_EQ(remote::refusal(Command::RestartService, 7, "display", 200, 100), "expired");
    EXPECT_EQ(remote::refusal(Command::RestartService, 7, "display", 100, 100), std::nullopt);
}

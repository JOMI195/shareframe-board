#include "util/Subprocess.hpp"
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
SubprocessResult sh(const std::string& script, int timeoutSecs = 5)
{
    return Subprocess::run({"sh", "-c", script}, timeoutSecs);
}

template <typename F>
std::chrono::milliseconds timed(F&& f)
{
    const auto start = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
}
}

TEST(Subprocess, StdoutStderrAndExitCodeArriveSeparately)
{
    const auto r = sh("echo out; echo err >&2; exit 3");
    EXPECT_EQ(r.exitCode, 3);
    EXPECT_EQ(r.stdOut, "out\n");
    EXPECT_EQ(r.stdErr, "err\n");
}

// The board passes SSIDs and passwords as arguments: they must never be shell-interpreted.
TEST(Subprocess, ArgumentsReachTheCommandVerbatim)
{
    const auto r = Subprocess::run({"printf", "%s|", "a b", "$HOME", "'q'; rm -rf /", ""});
    EXPECT_EQ(r.stdOut, "a b|$HOME|'q'; rm -rf /||");
}

TEST(Subprocess, AMissingCommandIs127)
{
    const auto r = Subprocess::run({"no-such-shareframe-tool"});
    EXPECT_EQ(r.exitCode, 127);
}

TEST(Subprocess, AKilledChildIsAFailure)
{
    EXPECT_NE(sh("kill -9 $$").exitCode, 0);
}

TEST(Subprocess, MegabytesOfOutputOnBothStreamsDoNotDeadlock)
{
    const auto r = sh("head -c 3000000 /dev/zero | tr '\\0' x; head -c 2000000 /dev/zero | tr '\\0' y >&2", 20);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.stdOut.size(), 3000000u);
    EXPECT_EQ(r.stdErr.size(), 2000000u);
}

TEST(Subprocess, AHungCommandIsKilledAtItsTimeout)
{
    SubprocessResult r{};
    const auto took = timed([&] { r = sh("echo started; sleep 30", 1); });
    EXPECT_EQ(r.exitCode, -1);
    EXPECT_EQ(r.stdOut, "started\n");
    EXPECT_EQ(r.stdErr, "timeout after 1s");
    EXPECT_LT(took, 3s);
}

// A command that detaches its output (as daemons and `exec >/dev/null` scripts do) must not
// outlive its timeout either.
TEST(Subprocess, ACommandThatClosedItsOutputIsStillKilledAtItsTimeout)
{
    SubprocessResult r{};
    const auto took = timed([&] { r = sh("exec >/dev/null 2>&1; sleep 30", 1); });
    EXPECT_EQ(r.exitCode, -1);
    EXPECT_LT(took, 3s);
}

// A tool must not hold the dashboard's listening socket or the database open after the service is gone.
TEST(Subprocess, AChildHoldsNothingButItsStandardStreams)
{
    const int inherited = open("/dev/null", O_RDONLY);
    ASSERT_GE(inherited, 3);
    const auto r = sh("for f in /proc/self/fd/*; do n=${f##*/}; [ \"$n\" -gt 2 ] && [ -e \"$f\" ] && echo $n; done; true");
    close(inherited);
    EXPECT_EQ(r.exitCode, 0) << r.stdErr;
    EXPECT_EQ(r.stdOut, "") << "open in the child";
}

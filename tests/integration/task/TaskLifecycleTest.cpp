#include "support/Wait.hpp"
#include "task/PeriodicTask.hpp"
#include <atomic>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class FlakyTask : public Task
{
public:
    using Task::Task;
    std::atomic<int> runs{0};

protected:
    void _run(std::stop_token st) override
    {
        if (++runs == 1)
            throw std::runtime_error("first run fails");
        std::unique_lock lk(mtx_);
        cv_.wait(lk, st, [] { return false; });
    }
};

class CountingTask : public PeriodicTask
{
public:
    CountingTask(EventBus& bus, const AppConfig& cfg, bool throws)
        : PeriodicTask(bus, cfg, "Counting"), throws_(throws) {}

    std::atomic<int> executions{0};

protected:
    int intervalSecs() const override { return 3600; }

    void execute() override
    {
        ++executions;
        if (throws_)
            throw std::runtime_error("execute fails");
    }

private:
    bool throws_;
};
}

TEST(Task, CrashedRunIsRestartedAfterBackoff)
{
    EventBus bus;
    FlakyTask task(bus, "Flaky");
    task.start();
    EXPECT_TRUE(waitFor([&] { return task.runs == 2; }, 3000ms));
    task.stop();
}

TEST(PeriodicTask, RunsOnStartAndOnDemandWithoutWaitingForTheInterval)
{
    EventBus bus;
    AppConfig cfg;
    CountingTask task(bus, cfg, false);
    task.start();
    ASSERT_TRUE(waitFor([&] { return task.executions == 1; }));

    task.runNow();
    EXPECT_TRUE(waitFor([&] { return task.executions == 2; }));
    task.stop();
}

TEST(PeriodicTask, FailingExecuteKeepsTheSchedule)
{
    EventBus bus;
    AppConfig cfg;
    CountingTask task(bus, cfg, true);
    task.start();
    ASSERT_TRUE(waitFor([&] { return task.executions == 1; }));

    task.runNow();
    EXPECT_TRUE(waitFor([&] { return task.executions == 2; }));
    task.stop();
}

TEST(PeriodicTask, StopInterruptsTheIntervalWait)
{
    EventBus bus;
    AppConfig cfg;
    CountingTask task(bus, cfg, false);
    task.start();
    ASSERT_TRUE(waitFor([&] { return task.executions == 1; }));

    const auto start = std::chrono::steady_clock::now();
    task.stop();
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
}

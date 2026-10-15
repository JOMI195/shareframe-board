#include "events/EventBus.hpp"
#include "support/Wait.hpp"
#include <atomic>
#include <gtest/gtest.h>

TEST(EventBus, DeliversToAllSubscribersOfTopicOnly)
{
    EventBus bus;
    std::atomic<int64_t> a{0}, b{0};
    std::atomic<int> skips{0};
    bus.subscribe<Topic::IMAGE_NEW>([&](const int64_t& id) { a = id; });
    bus.subscribe<Topic::IMAGE_NEW>([&](const int64_t& id) { b = id; });
    bus.subscribe<Topic::SKIP_IMAGE>([&](const SkipImageEvent&) { ++skips; });
    bus.start();

    bus.publish<Topic::IMAGE_NEW>(7);
    EXPECT_TRUE(waitFor([&] { return a == 7 && b == 7; }));
    EXPECT_EQ(skips, 0);
}

TEST(EventBus, PreservesPublishOrder)
{
    EventBus bus;
    std::mutex m;
    std::vector<int64_t> seen;
    bus.subscribe<Topic::IMAGE_NEW>([&](const int64_t& id) { std::lock_guard lk(m); seen.push_back(id); });
    bus.start();
    for (int64_t i = 0; i < 100; ++i)
        bus.publish<Topic::IMAGE_NEW>(i);

    ASSERT_TRUE(waitFor([&] { std::lock_guard lk(m); return seen.size() == 100; }));
    for (int64_t i = 0; i < 100; ++i)
        EXPECT_EQ(seen[i], i);
}

TEST(EventBus, ThrowingHandlerDoesNotStopOthers)
{
    EventBus bus;
    std::atomic<bool> reached{false};
    bus.subscribe<Topic::IMAGE_NEW>([](const int64_t&) { throw std::runtime_error("boom"); });
    bus.subscribe<Topic::IMAGE_NEW>([&](const int64_t&) { reached = true; });
    bus.start();
    bus.publish<Topic::IMAGE_NEW>(1);
    EXPECT_TRUE(waitFor([&] { return reached.load(); }));
}

TEST(EventBus, StopWithoutStartIsSafe)
{
    EventBus bus;
    bus.stop();
}

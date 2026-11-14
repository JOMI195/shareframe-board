#include "events/EventBus.hpp"
#include "support/TempDir.hpp"
#include "task/WifiReturnWatcher.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class Watcher : public WifiReturnWatcher
{
public:
    using WifiReturnWatcher::WifiReturnWatcher;
    using WifiReturnWatcher::execute;
};

class WifiReturnWatcherTest : public ::testing::Test
{
protected:
    WifiReturnWatcherTest() { cfg.display.wifiModePath = (dir / "wifi-mode.json").string(); }

    void publish(const std::string& mode) const
    {
        std::ofstream(cfg.display.wifiModePath) << R"({"mode":")" << mode << R"("})";
    }

    TempDir dir;
    AppConfig cfg;
    EventBus bus;
    int returns = 0;
    Watcher watcher{bus, cfg, [this] { ++returns; }};
};
}

TEST_F(WifiReturnWatcherTest, CallsBackWhenTheFrameJoinsANetworkAgain)
{
    publish("connected");
    watcher.execute();
    publish("connecting");
    watcher.execute();
    publish("ap");
    watcher.execute();
    EXPECT_EQ(returns, 0);

    publish("connected");
    watcher.execute();
    EXPECT_EQ(returns, 1);
    watcher.execute();
    EXPECT_EQ(returns, 1) << "staying connected is no return";
}

TEST_F(WifiReturnWatcherTest, TheFirstLookIsNoReturn)
{
    publish("connected");
    watcher.execute();
    EXPECT_EQ(returns, 0);
}

TEST_F(WifiReturnWatcherTest, AMissingModeFileCountsAsAway)
{
    watcher.execute();
    publish("connected");
    watcher.execute();
    EXPECT_EQ(returns, 1);
}
